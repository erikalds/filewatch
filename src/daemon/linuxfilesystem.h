#ifndef LINUXFILESYSTEM_H
#define LINUXFILESYSTEM_H

#ifdef __linux__

#  include "common/loop_thread.h"
#  include "defaultfilesystem.h"
#  include "directoryeventlistener.h"
#  include <limits.h>
#  include <poll.h>
#  include <sys/inotify.h>
#  include <unistd.h>

#  include <spdlog/spdlog.h>

#  include <functional>
#  include <map>
#  include <set>

struct inotify_event;

namespace threading
{
  class LoopThread;
  class LoopThreadFactory;
}  // namespace threading

namespace fw
{
  namespace dm
  {
    namespace dtls
    {
      class Inotify
      {
      protected:
        Inotify();
        void init();

      public:
        template<typename T>
        static std::unique_ptr<Inotify> create()
        {
          std::unique_ptr<Inotify> ptr{new T};
          ptr->init();
          return ptr;
        }

        virtual ~Inotify();
        int close();

        int add_watch(const char* pathname, uint32_t mask)
        {
          int rv = syscall_inotify_add_watch(inotify_fd, pathname, mask);
          // wake up poll thread so that it can poll on the newly added wds
          char buf = static_cast<char>(1);
          syscall_write(pipe_fds[1], &buf, 1);
          return rv;
        }
        int rm_watch(int wd)
        {
          int rv = syscall_inotify_rm_watch(inotify_fd, wd);
          // wake up poll thread so that it can poll on the remaining wds
          char buf = static_cast<char>(2);
          syscall_write(pipe_fds[1], &buf, 1);
          return rv;
        }
        ssize_t read(void* buf, size_t count)
        {
          return syscall_read(inotify_fd, buf, count);
        }
        void terminate_poll()
        {
          char buf = static_cast<char>(0);
          syscall_write(pipe_fds[1], &buf, 1);
        }
        int poll(short events, short& revents, int timeout_ms)
        {
          struct pollfd pfd[2];
          pfd[0].fd = inotify_fd;
          pfd[0].events = events;
          pfd[1].fd = pipe_fds[0];
          pfd[1].events = events;
          short expected_events = events | POLLHUP | POLLERR | POLLNVAL;
          while (true)
          {
            int poll_result = syscall_poll(pfd, 2, timeout_ms);
            if ((pfd[0].revents & expected_events) != 0
                || poll_result == -1)
            {
              revents = pfd[0].revents;
              return poll_result;
            }
            else if ((pfd[1].revents & expected_events) != 0)
            {
              char buf;
              syscall_read(pipe_fds[0], &buf, 1); // remove written char to pipe
              if (static_cast<int>(buf) == 0)
              {
                spdlog::debug("read termination signal from pipe");
                errno = EINTR;
                return -1;
              }
              else if (static_cast<int>(buf) == 1)
              {
                spdlog::debug("read add signal from pipe - poll again");
              }
              else if (static_cast<int>(buf) == 2)
              {
                spdlog::debug("read rm signal from pipe - poll again");
              }
              else
              {
                spdlog::debug("read unknown signal from pipe: {} - poll again",
                              static_cast<int>(buf));
              }
            }
          }
        }

      private:
        virtual int syscall_inotify_init();
        virtual int syscall_close(int fd);

        virtual int syscall_inotify_add_watch(int fd, const char* pathname,
                                              uint32_t mask);
        virtual int syscall_inotify_rm_watch(int fd, int wd);
        virtual ssize_t syscall_read(int fd, void* buf, size_t count);
        virtual int syscall_poll(struct pollfd* fds, nfds_t nfds,
                                 int timeout_ms);
        virtual int syscall_pipe2(std::array<int, 2>& pipefd, int flags);
        virtual ssize_t syscall_write(int fd, const void* buf, size_t count);

        int inotify_fd = -1;
        std::array<int, 2> pipe_fds = { 0, 0 };
      };

      class Watch
      {
      public:
        Watch(Inotify& inotify_, const std::filesystem::path& fullpath,
              const std::deque<fs::DirectoryEntry>& direntries);
        Watch(const Watch&) = delete;
        Watch& operator=(const Watch&) = delete;
        Watch(Watch&&) noexcept;
        Watch& operator=(Watch&&) noexcept;
        ~Watch();

        void add_listener(DirectoryEventListener& listener);
        bool remove_listener(DirectoryEventListener& listener);

        bool event(std::string_view containing_dir,
                   const std::function<std::optional<fs::DirectoryEntry>()>&
                     get_direntry,
                   inotify_event* evt) const;

      private:
        Inotify& inotify;
        int wd;
        std::set<DirectoryEventListener*> listeners;
        mutable std::set<std::string> directories;
      };

    }  // namespace dtls

    template<typename SuperClassT>
    class OSFileSystem : public SuperClassT
    {
    public:
      explicit OSFileSystem(
        std::string_view rootdir,
        std::unique_ptr<dtls::Inotify> in_ptr = nullptr,
        std::unique_ptr<threading::LoopThreadFactory> thr_fac = nullptr);
      ~OSFileSystem();

      void watch(std::string_view dirname,
                 DirectoryEventListener& listener) override;
      void stop_watching(std::string_view dirname,
                         DirectoryEventListener& listener) override;

    private:
      void poll_watches();

      std::unique_ptr<dtls::Inotify> inotify;
      std::map<std::string, dtls::Watch> watches;
      std::unique_ptr<threading::LoopThreadFactory> thread_factory;
      std::unique_ptr<threading::LoopThread> watch_thread;
      std::atomic<int> currently_polling = 0;
    };

  }  // namespace dm
}  // namespace fw


template<typename SuperClassT>
fw::dm::OSFileSystem<SuperClassT>::OSFileSystem(
  std::string_view rootdir_,
  std::unique_ptr<dtls::Inotify>
    in_ptr,
  std::unique_ptr<threading::LoopThreadFactory>
    thr_fac) :
  SuperClassT(rootdir_),
  inotify(), watches(), thread_factory()
{
  if (in_ptr == nullptr)
    in_ptr = dtls::Inotify::create<dtls::Inotify>();

  if (thr_fac == nullptr)
    thr_fac = threading::create_thread_factory();

  inotify = std::move(in_ptr);
  thread_factory = std::move(thr_fac);

  watch_thread = thread_factory->create_thread([&]() { this->poll_watches(); },
                                               true /* create_suspended */);
}

template<typename SuperClassT>
fw::dm::OSFileSystem<SuperClassT>::~OSFileSystem()
{
  watches.clear();
  watch_thread->stop();

  if (watch_thread->is_joinable())
  {
    if (currently_polling > 0)
    {
      inotify->terminate_poll();
    }
    spdlog::debug("watch_thread->join()");
    watch_thread->join();
  }

  if (inotify->close() == -1)
  {
    auto pre = "close(inotify_fd) failed: ";
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

template<typename SuperClassT>
void fw::dm::OSFileSystem<SuperClassT>::watch(std::string_view dirname,
                                              DirectoryEventListener& listener)
{
  const std::string dn{dirname};
  try
  {
    auto unsuspend_watch_thread = watches.empty();

    auto iter = watches.find(dn);
    if (iter == std::end(watches))
    {
      dtls::Watch w{*inotify, this->join(this->rootdir.string(), dirname),
                    this->ls(dirname)};
      iter = watches.insert(std::make_pair(dn, std::move(w))).first;
    }
    iter->second.add_listener(listener);
    listener.notify(filewatch::DirectoryEvent::WATCHING_DIRECTORY,
                    dirname, ".", 0);

    if (unsuspend_watch_thread)
    {
      watch_thread->unsuspend();
      spdlog::debug("unsuspended watch thread");
    }
  }
  catch (const std::runtime_error& e)
  {
    throw std::runtime_error(dn + " error: " + e.what());
  }
}

template<typename SuperClassT>
void fw::dm::OSFileSystem<SuperClassT>::stop_watching(
  std::string_view dirname, DirectoryEventListener& listener)
{
  auto iter = watches.find(std::string(dirname));
  if (iter == std::end(watches))
  {
    return;
  }

  if (iter->second.remove_listener(listener))
  {
    watches.erase(iter);
  }

  if (watches.empty())
  {
    watch_thread->suspend();
    spdlog::debug("suspended watch thread");
  }
}

namespace
{
  template<int alignment, typename T>
  T* align_ptr(T* ptr, std::size_t& wanted_size)
  {
    void* vptr = reinterpret_cast<void*>(ptr);
    void* alignedvptr = std::align(alignment, sizeof(T), vptr, wanted_size);
    return reinterpret_cast<T*>(alignedvptr);
  }

}  // namespace

template<typename SuperClassT>
void fw::dm::OSFileSystem<SuperClassT>::poll_watches()
{
  constexpr const std::size_t maxstructsize =
    sizeof(inotify_event) + NAME_MAX + 1 + 4;
  char buf[maxstructsize];
  auto actualsize = maxstructsize;
  char* alignedbuf = align_ptr<4>(buf, actualsize);

  spdlog::debug("poll_watches...");
  short revents = 0;
  currently_polling = 1;
  int event_count = inotify->poll(POLLIN, revents, -1);
  currently_polling = 0;
  if (event_count == -1)
  {
    auto pre = "poll error: ";
    switch (errno)
    {
    case EFAULT:
      spdlog::error("poll: EFAULT");
      throw std::runtime_error(fmt::format("{}fds points outside the process's accessible address space. The array given as argument was not contained in the calling program's address space.", pre));

    case EINTR:
      spdlog::info("poll: interrupted by signal.", pre);
      return;

    case EINVAL:
      spdlog::error("poll: EINVAL");
      throw std::runtime_error(fmt::format("{}The nfds value exceeds the RLIMIT_NOFILE value or the timeout value expressed in *ip is invalid (negative).", pre));

    case ENOMEM:
      spdlog::error("poll: ENOMEM");
      throw std::runtime_error(fmt::format("{}Unable to allocate memory for kernel data structures.", pre));
    default:
      spdlog::error("poll: unknown errno value");
      throw std::runtime_error(fmt::format("{}Unknown errno value.", pre));
    }
  }
  spdlog::debug("poll_watches -> {} [0x{:x}]", event_count, revents);

  auto len = inotify->read(alignedbuf, actualsize);
  spdlog::debug("read {} bytes", len);
  inotify_event* event = nullptr;
  for (char* ptr = alignedbuf; ptr < alignedbuf + len;
       ptr += sizeof(inotify_event) + event->len)
  {
    event = reinterpret_cast<inotify_event*>(ptr);
    spdlog::debug("processing event: {}, {} registered watche(s).", event->wd,
                  watches.size());

    std::string_view filename{event->name,
                              std::min(static_cast<std::size_t>(event->len),
                                       std::strlen(event->name))};
    for (auto& w : watches)
    {
      if (w.second.event(
            w.first,
            [&]() {
              auto containing_dir = w.first;
              return this->get_direntry(this->join(containing_dir, filename));
            },
            event))
      {
        spdlog::debug("event processed");
        break;
      }
    }
  }
}

#endif  // __linux__

#endif /* LINUXFILESYSTEM_H */
