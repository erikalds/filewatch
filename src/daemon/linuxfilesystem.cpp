#include "linuxfilesystem.h"

#ifdef __linux__

#  include "common/loop_thread.h"
#  include "directoryeventlistener.h"


fw::dm::dtls::Watch::Watch(Inotify& inotify_,
                           const std::filesystem::path& fullpath,
                           const std::deque<fs::DirectoryEntry>& direntries) :
  inotify(inotify_),
  wd(inotify.add_watch(fullpath.string().c_str(),
                       // use of a signed integer operand with a binary bitwise
                       // operator [hicpp-signed-bitwise]
                       IN_CLOSE_WRITE  // NOLINT
                         | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF
                         | IN_MOVED_FROM | IN_MOVED_TO))
{
  std::ostringstream ost;
  for (const auto& de : direntries)
  {
    if (de.is_dir)
    {
      directories.insert(de.name);
      ost << " " << de.name;
    }
  }

  if (wd == -1)
  {
    switch (errno)
    {
    case EACCES:
      throw std::runtime_error(
        "Read access to the given file is not permitted [EACCES].");
    case EBADF:
      throw std::runtime_error(
        "The given file descriptor is not valid [EBADF].");
    case EFAULT:
      throw std::runtime_error(
        "pathname points outside of the process's accessible address space "
        "[EFAULT].");
    case EINVAL:
      throw std::runtime_error(
        "The given event mask contains no valid events; or mask contains both "
        "IN_MASK_ADD and IN_MASK_CREATE; or fd is not an inotify file "
        "descriptor [EINVAL].");
    case ENAMETOOLONG:
      throw std::runtime_error("pathname is too long [ENAMETOOLONG].");
    case ENOENT:
      throw std::runtime_error(
        "A directory component in pathname does not exist or is a dangling "
        "symbolic link [ENOENT].");
    case ENOMEM:
      throw std::runtime_error(
        "Insufficient kernel memory was available [ENOMEM].");
    case ENOSPC:
      throw std::runtime_error(
        "The user limit on the total number of inotify watches was reached or "
        "the kernel failed to allocate a needed resource [ENOSPC].");
    case ENOTDIR:
      throw std::runtime_error(
        "mask contains IN_ONLYDIR and pathname is not a directory [ENOTDIR].");
    case EEXIST:
      throw std::runtime_error(
        "mask contains IN_MASK_CREATE and pathname refers to a file already "
        "being watched by the same fd [EEXIST].");
    default:
    {
      std::ostringstream err;
      err << "Unknown error code [" << errno << "].";
      throw std::runtime_error(err.str());
    }
    }
  }
  spdlog::info("Watching {} [{}] [{} ]", fullpath.string(), wd, ost.str());
}

fw::dm::dtls::Watch::Watch(Watch&& other) noexcept :
  inotify(other.inotify), wd(other.wd), listeners(std::move(other.listeners)),
  directories(std::move(other.directories))
{
  other.wd = -1;
}

fw::dm::dtls::Watch& fw::dm::dtls::Watch::operator=(Watch&& other) noexcept
{
  if (&other != this)
  {
    inotify = other.inotify;
    wd = other.wd;
    listeners = std::move(other.listeners);
    directories = std::move(other.directories);

    other.wd = -1;
  }
  return *this;
}

fw::dm::dtls::Watch::~Watch()
{
  if (wd != -1)
  {
    inotify.rm_watch(wd);
    spdlog::info("rm_watch [{}]", wd);
  }
}

void fw::dm::dtls::Watch::add_listener(DirectoryEventListener& listener)
{
  listeners.insert(&listener);
}

bool fw::dm::dtls::Watch::remove_listener(DirectoryEventListener& listener)
{
  listeners.erase(&listener);
  return listeners.empty();
}

namespace
{
  std::optional<filewatch::DirectoryEvent::Event>
  choose_event_type(inotify_event& evt, bool is_dir)
  {
    if ((evt.mask & IN_CREATE) != 0)  // NOLINT
    {
      return is_dir ? filewatch::DirectoryEvent::DIRECTORY_ADDED
                    : filewatch::DirectoryEvent::FILE_ADDED;
    }
    if ((evt.mask & IN_DELETE) != 0)  // NOLINT
    {
      return is_dir ? filewatch::DirectoryEvent::DIRECTORY_REMOVED
                    : filewatch::DirectoryEvent::FILE_REMOVED;
    }

    return std::optional<filewatch::DirectoryEvent::Event>();
  }

  std::string masktostr(uint32_t mask)
  {
    static std::map<uint32_t, std::string> maskstrs{
      {IN_ACCESS, "IN_ACCESS"},
      {IN_ATTRIB, "IN_ATTRIB"},
      {IN_CLOSE_WRITE, "IN_CLOSE_WRITE"},
      {IN_CLOSE_NOWRITE, "IN_CLOSE_NOWRITE"},
      {IN_CREATE, "IN_CREATE"},
      {IN_DELETE, "IN_DELETE"},
      {IN_DELETE_SELF, "IN_DELETE_SELF"},
      {IN_MODIFY, "IN_MODIFY"},
      {IN_MOVE_SELF, "IN_MOVE_SELF"},
      {IN_MOVED_FROM, "IN_MOVED_FROM"},
      {IN_MOVED_TO, "IN_MOVED_TO"},
      {IN_OPEN, "IN_OPEN"}};
    std::ostringstream ost;
    for (const auto& p : maskstrs)
    {
      if ((mask & p.first) != 0)
      {
        ost << " " << p.second;
      }
    }
    return ost.str();
  }

}  // anonymous namespace

bool fw::dm::dtls::Watch::event(
  std::string_view containing_dir,
  const std::function<std::optional<fs::DirectoryEntry>()>& get_direntry,
  inotify_event* evt) const
{
  if (evt->wd != wd)
  {
    return false;
  }

  std::string filename{evt->name,  // NOLINT
                       std::min(static_cast<std::size_t>(evt->len),
                                std::strlen(evt->name))};  // NOLINT
  uint64_t mtime = 0;
  auto pre = fmt::format("event:{} (0x{:x}), {}, {}, I have {} listeners",
                         masktostr(evt->mask), evt->mask, evt->cookie, filename,
                         listeners.size());

  auto direntry = get_direntry();
  bool is_dir = false;
  if (direntry)
  {
    mtime = direntry->mtime;
    filename = direntry->name;
    is_dir = direntry->is_dir;
  }
  else
  {
    is_dir = directories.find(filename) != std::end(directories);
    spdlog::debug("{}, but my direntry was empty [isdir: {}].", pre, is_dir);
  }

  auto event_type = choose_event_type(*evt, is_dir);
  if (!event_type)
  {
    spdlog::warn("{}. Unsupported event mask.", pre);
    return false;
  }

  spdlog::debug("{}, and my {}name is \"{}\".", pre,
                is_dir ? "directory " : "file", filename);

  if (*event_type == filewatch::DirectoryEvent::DIRECTORY_ADDED)
  {
    spdlog::debug("Directory added: {}", filename);
    directories.insert(filename);
  }
  else if (*event_type == filewatch::DirectoryEvent::DIRECTORY_REMOVED)
  {
    spdlog::debug("Directory removed: {}", filename);
    directories.erase(filename);
  }

  spdlog::info("notify {} listeners about {}/{}: {}", listeners.size(),
               containing_dir, filename, *event_type);
  for (auto* listener : listeners)
  {
    listener->notify(*event_type, containing_dir, filename, mtime);
  }
  return true;
}

#endif  // __linux__
