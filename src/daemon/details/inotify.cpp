#include "daemon/details/inotify.h"

#ifdef __linux__

#  include "common/bw_combine.h"
#  include <fcntl.h>
#  include <sys/inotify.h>
#  include <unistd.h>

#  include <sstream>

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
  if (syscall_pipe2(pipe_fds, O_CLOEXEC) == -1)
  {
    std::ostringstream ost;
    ost << "Could not initialize pipe: ";
    switch (errno)
    {
    case EFAULT:
      ost << "pipefd is not valid [EFAULT].";
      break;

    case EINVAL:
      ost << "(pipe2()) Invalid value in flags [EINVAL].";
      break;

    case EMFILE:
      ost << "The per-process limit on the number of open file descriptors\n"
          << "has been reached [EMFILE].";
      break;

    case ENFILE:
      ost << "The system-wide limit on the total number of open files has\n"
          << "been reached [ENFILE].";
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

int fw::dm::dtls::Inotify::add_watch(const char* pathname, uint32_t mask)
{
  int rv = syscall_inotify_add_watch(inotify_fd, pathname, mask);
  // wake up poll thread so that it can poll on the newly added wds
  char buf = static_cast<char>(1);
  syscall_write(pipe_fds[1], &buf, 1);
  return rv;
}

int fw::dm::dtls::Inotify::rm_watch(int wd)
{
  int rv = syscall_inotify_rm_watch(inotify_fd, wd);
  // wake up poll thread so that it can poll on the remaining wds
  char buf = static_cast<char>(2);
  syscall_write(pipe_fds[1], &buf, 1);
  return rv;
}

ssize_t fw::dm::dtls::Inotify::read(void* buf, size_t count) noexcept
{
  return syscall_read(inotify_fd, buf, count);
}

void fw::dm::dtls::Inotify::terminate_poll()
{
  char buf = static_cast<char>(0);
  syscall_write(pipe_fds[1], &buf, 1);
}

namespace
{
  bool has_event(const struct pollfd& pfd, unsigned short expected_events)
  {
    return (static_cast<unsigned short>(pfd.revents) & expected_events) != 0;
  }

}  // anonymous namespace

int fw::dm::dtls::Inotify::poll(short events, short& revents,
                                int timeout_ms) noexcept
{
  std::array<struct pollfd, 2> pfd{};
  pfd[0].fd = inotify_fd;
  pfd[0].events = events;
  pfd[1].fd = pipe_fds[0];
  pfd[1].events = events;
  auto expected_events =
    util::bw_combine<unsigned short>(events, POLLHUP, POLLERR, POLLNVAL);
  while (true)
  {
    int poll_result = syscall_poll(pfd.data(), 2, timeout_ms);
    if (has_event(pfd[0], expected_events) || poll_result == -1)
    {
      revents = pfd[0].revents;
      return poll_result;
    }

    if (has_event(pfd[1], expected_events))
    {
      char buf = 0;
      syscall_read(pipe_fds[0], &buf, 1);  // remove written char from pipe
      if (static_cast<int>(buf) == 0)
      {
        spdlog::debug("read termination signal from pipe");
        errno = EINTR;
        return -1;
      }

      if (static_cast<int>(buf) == 1)
      {
        spdlog::debug("read add signal from pipe - poll again");
      }
      else if (static_cast<int>(buf) == 2)
      {
        spdlog::debug("read rm signal from pipe - poll again");
      }
      else
      {
        spdlog::warn("read unknown signal from pipe: {} - poll again",
                     static_cast<int>(buf));
      }
    }
  }
}

int fw::dm::dtls::Inotify::syscall_inotify_init()
{
  return inotify_init1(IN_CLOEXEC);
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

ssize_t fw::dm::dtls::Inotify::syscall_read(int fd, void* buf,
                                            size_t count) noexcept
{
  return ::read(fd, buf, count);
}

int fw::dm::dtls::Inotify::syscall_poll(struct pollfd* fds, nfds_t nfds,
                                        int timeout_ms)
{
  return ::poll(fds, nfds, timeout_ms);
}

int fw::dm::dtls::Inotify::syscall_pipe2(std::array<int, 2>& pipefd, int flags)
{
  return ::pipe2(pipefd.data(), flags);
}

ssize_t fw::dm::dtls::Inotify::syscall_write(int fd, const void* buf,
                                             size_t count)
{
  return ::write(fd, buf, count);
}

#endif  // __linux__
