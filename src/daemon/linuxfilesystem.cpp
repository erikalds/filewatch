#include "linuxfilesystem.h"

#ifdef __linux__

#  include "common/bw_combine.h"
#  include "common/loop_thread.h"
#  include "directoryeventlistener.h"

#  include <sys/inotify.h>

fw::dm::dtls::Watch::Watch(Inotify& inotify_,
                           const std::filesystem::path& fullpath,
                           const std::deque<fs::DirectoryEntry>& direntries) :
  inotify(inotify_),
  wd(inotify.add_watch(fullpath.string().c_str(),
                       util::bw_combine<unsigned short>(
                         IN_CLOSE_WRITE, IN_CREATE, IN_DELETE, IN_DELETE_SELF,
                         IN_MOVE_SELF, IN_MOVED_FROM, IN_MOVED_TO)))
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

bool fw::dm::dtls::Watch::event(std::string_view containing_dir,
                                std::string filename,
                                const GetDirEntryFun& get_direntry,
                                inotify_event* evt) const
{
  if (evt->wd != wd)
  {
    return false;
  }

  uint64_t mtime = 0;
  auto pre = fmt::format("event:{} (0x{:x}), {}, {}, I have {} listeners",
                         masktostr(evt->mask), evt->mask, evt->cookie, filename,
                         listeners.size());

  auto direntry = get_direntry(containing_dir, filename);
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

std::optional<short>
fw::dm::dtls::safe_poll_inotify(Inotify& inotify,
                                std::atomic<int>& currently_polling) noexcept
{
  spdlog::debug("poll_watches...");
  short revents = 0;
  currently_polling = 1;
  int event_count = inotify.poll(POLLIN, revents, -1); // -1 => block forever
  currently_polling = 0;
  if (event_count == -1)
  {
    const auto* pre = "poll error: ";
    switch (errno)
    {
    case EFAULT:
      spdlog::error("{}fds points outside the process's accessible address\n"
                    "space. The array given as argument was not contained in\n"
                    "the calling program's address space. [EFAULT]",
                    pre);
      break;

    case EINTR:
      spdlog::info("poll: interrupted by signal.", pre);
      break;

    case EINVAL:
      spdlog::error("{}The nfds value exceeds the RLIMIT_NOFILE value or the\n"
                    "timeout value expressed in *ip is invalid (negative).\n"
                    "[EINVAL]", pre);
      break;

    case ENOMEM:
      spdlog::error("{}Unable to allocate memory for kernel data structures.\n"
                    "[ENOMEM]", pre);
      break;

    default:
      spdlog::error("{}unknown errno value [{}]", pre, errno);
      break;
    }
    return std::optional<short>();
  }
  spdlog::debug("poll_watches -> {} [0x{:x}]", event_count, revents);
  return revents;
}

namespace
{
  template<int alignment, typename T>
  T* align_ptr(T* ptr, std::size_t& wanted_size)
  {
    void* vptr = reinterpret_cast<void*>(ptr); // NOLINT - reinterpret_cast
    void* alignedvptr = std::align(alignment, sizeof(T), vptr, wanted_size);
    return reinterpret_cast<T*>(alignedvptr); // NOLINT - reinterpret_cast
  }

}  // namespace

void fw::dm::dtls::process_inotify_events(Inotify& inotify,
                                          const std::map<std::string, dtls::Watch>& watches,
                                          const fw::dm::dtls::GetDirEntryFun& get_direntry) noexcept
{
  constexpr const std::size_t maxstructsize =
    sizeof(inotify_event) + NAME_MAX + 1 + 4;
  std::array<char, maxstructsize> buf{};
  auto actualsize = maxstructsize;
  char* alignedbuf = align_ptr<4>(buf.data(), actualsize);

  auto len = inotify.read(alignedbuf, actualsize);
  spdlog::debug("read {} bytes", len);
  inotify_event* event = nullptr;
  // NOLINTNEXTLINE - pointer arithmetic
  for (char* ptr = alignedbuf; ptr < alignedbuf + len;
       ptr += sizeof(inotify_event) + event->len) // NOLINT - ptr arithmetic
  {
    event = reinterpret_cast<inotify_event*>(ptr); // NOLINT - reinterpret_cast
    std::string filename{event->name,  // NOLINT
                         std::min(static_cast<std::size_t>(event->len),
                                  std::strlen(event->name))};  // NOLINT
    spdlog::debug("processing event: {}, {} registered watche(s).", event->wd,
                  watches.size());

    for (const auto& w : watches)
    {
      if (w.second.event(w.first, filename, get_direntry, event))
      {
        spdlog::debug("event processed");
        break;
      }
    }
  }
}

void fw::dm::dtls::close_inotify(Inotify& inotify) noexcept
{
  if (inotify.close() == -1)
  {
    const auto* pre = "close(inotify_fd) failed: ";
    switch (errno)
    {
    case EBADF:
      spdlog::warn("{}inotify_fd is not a valid open file descriptor.", pre);
      break;
    case EINTR:
      spdlog::warn("{}close was interrupted by a signal.", pre);
      break;
    case EIO:
      spdlog::warn("{}An I/O error occurred.", pre);
      break;
    case ENOSPC:
      spdlog::warn("{}No space left on device.", pre);
      break;
    case EDQUOT:
      spdlog::warn("{}Disk quota exceeded.", pre);
      break;
    default:
      spdlog::warn("{}Unknown reason.", pre);
      break;
    }
  }
}

#endif  // __linux__
