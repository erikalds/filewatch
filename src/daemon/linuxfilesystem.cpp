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
                           const std::filesystem::path& fullpath) :
  inotify(inotify_),
  wd(inotify.add_watch(fullpath.string().c_str(),
                       // use of a signed integer operand with a binary bitwise
                       // operator [hicpp-signed-bitwise]
                       IN_CLOSE_WRITE  // NOLINT
                         | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF
                         | IN_MOVED_FROM | IN_MOVED_TO))
{
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
      std::ostringstream ost;
      ost << "Unknown error code [" << errno << "].";
      throw std::runtime_error(ost.str());
    }
    }
  }
}

fw::dm::dtls::Watch::Watch(Watch&& other) noexcept :
  inotify(other.inotify), wd(other.wd), listeners(std::move(other.listeners))
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

    other.wd = -1;
  }
  return *this;
}

fw::dm::dtls::Watch::~Watch()
{
  if (wd != -1)
  {
    inotify.rm_watch(wd);
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
  auto choose_event_type(inotify_event& evt)
  {
    switch (evt.mask)
    {
    case IN_CREATE:
      return filewatch::DirectoryEvent::FILE_ADDED;
    case IN_DELETE:
      return filewatch::DirectoryEvent::FILE_REMOVED;
    default:
      assert(false);
      return filewatch::DirectoryEvent::FILE_ADDED;
    }
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

  auto direntry = get_direntry();
  auto pre = fmt::format(
    "event: 0x{:x}, {}, {}, I have {} listeners", evt->mask, evt->cookie,
    std::string_view(static_cast<char*>(evt->name), evt->len),
    listeners.size());

  if (!direntry)
  {
    spdlog::warn("{}, but my direntry was empty.", pre);
    return false;
  }

  spdlog::debug("{}, and my direntry has the name \"{}\".\n", pre,
                direntry->name);

  auto event_type = choose_event_type(*evt);
  for (auto* listener : listeners)
  {
    listener->notify(event_type, containing_dir, direntry->name,
                     direntry->mtime);
  }
  return true;
}

#endif  // __linux__
