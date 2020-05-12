#include "linuxfilesystem.h"

#ifdef __linux__

#  include "common/loop_thread.h"
#  include "directoryeventlistener.h"
#  include <sys/inotify.h>
#  include <unistd.h>

#  include <cassert>
#  include <climits>
#  include <iostream>
#  include <stdexcept>


fw::dm::dtls::Inotify::Inotify() = default;

void fw::dm::dtls::Inotify::init()
{
  inotify_fd = syscall_inotify_init();
  if (inotify_fd == -1)
  {
    std::ostringstream ost;
    ost << "Could not initialize inotify: ";
    switch (errno)
    {
    case EMFILE:
      ost << "Too many inotify instances or open file descriptors";
      break;
    case ENFILE:
      ost << "Too many open files";
      break;
    case ENOMEM:
      ost << "Insufficient kernel memory available";
      break;
    default:
      ost << "Unknown error";
      break;
    }
    throw std::runtime_error(ost.str());
  }
}

fw::dm::dtls::Inotify::~Inotify()
{
  assert(inotify_fd == -1);  // verify was closed
}

int fw::dm::dtls::Inotify::close()
{
  auto rv = syscall_close(inotify_fd);
  inotify_fd = -1;
  return rv;
}

int fw::dm::dtls::Inotify::syscall_inotify_init()
{
  return inotify_init();  // NOLINT
}

int fw::dm::dtls::Inotify::syscall_close(int fd) { return ::close(fd); }

int fw::dm::dtls::Inotify::syscall_inotify_add_watch(int fd,
                                                     const char* pathname,
                                                     uint32_t mask)
{
  return inotify_add_watch(fd, pathname, mask);
}

int fw::dm::dtls::Inotify::syscall_inotify_rm_watch(int fd, int wd)
{
  return inotify_rm_watch(fd, wd);
}

ssize_t fw::dm::dtls::Inotify::syscall_read(int fd, void* buf, size_t count)
{
  return ::read(fd, buf, count);
}

int fw::dm::dtls::Inotify::syscall_poll(struct pollfd* fds, nfds_t nfds,
                                        int timeout_ms)
{
  return ::poll(fds, nfds, timeout_ms);
}


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
  inotify(other.inotify),
  wd(other.wd),
  listeners(std::move(other.listeners)),
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
    if ((evt.mask & IN_CREATE) != 0)
    {
      return is_dir?
        filewatch::DirectoryEvent::DIRECTORY_ADDED
        : filewatch::DirectoryEvent::FILE_ADDED;
    }
    else if ((evt.mask & IN_DELETE) != 0)
    {
      return is_dir?
        filewatch::DirectoryEvent::DIRECTORY_REMOVED
        : filewatch::DirectoryEvent::FILE_REMOVED;
    }
    else
    {
      return std::optional<filewatch::DirectoryEvent::Event>();
    }
  }

  std::string masktostr(uint32_t mask)
  {
    std::ostringstream ost;
    if ((mask & IN_ACCESS) != 0) ost << " IN_ACCESS";
    if ((mask & IN_ATTRIB) != 0) ost << " IN_ATTRIB";
    if ((mask & IN_CLOSE_WRITE) != 0) ost << " IN_CLOSE_WRITE";
    if ((mask & IN_CLOSE_NOWRITE) != 0) ost << " IN_CLOSE_NOWRITE";
    if ((mask & IN_CREATE) != 0) ost << " IN_CREATE";
    if ((mask & IN_DELETE) != 0) ost << " IN_DELETE";
    if ((mask & IN_DELETE_SELF) != 0) ost << " IN_DELETE_SELF";
    if ((mask & IN_MODIFY) != 0) ost << " IN_MODIFY";
    if ((mask & IN_MOVE_SELF) != 0) ost << " IN_MOVE_SELF";
    if ((mask & IN_MOVED_FROM) != 0) ost << " IN_MOVED_FROM";
    if ((mask & IN_MOVED_TO) != 0) ost << " IN_MOVED_TO";
    if ((mask & IN_OPEN) != 0) ost << " IN_OPEN";
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

  std::string filename{evt->name,
                       std::min(static_cast<std::size_t>(evt->len),
                                std::strlen(evt->name))};
  uint64_t mtime = 0;
  auto pre = fmt::format("event:{} (0x{:x}), {}, {}, I have {} listeners",
                         masktostr(evt->mask), evt->mask, evt->cookie, filename, listeners.size());

  auto direntry = get_direntry();
  bool is_dir;
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

  spdlog::debug("{}, and my {}name is \"{}\".",
                pre, is_dir ? "directory " : "file", filename);

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
