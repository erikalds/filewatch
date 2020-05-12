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
          return syscall_inotify_add_watch(inotify_fd, pathname, mask);
        }
        int rm_watch(int wd)
        {
          return syscall_inotify_rm_watch(inotify_fd, wd);
        }
        ssize_t read(void* buf, size_t count)
        {
          return syscall_read(inotify_fd, buf, count);
        }
        int poll(short events, short& revents, int timeout_ms)
        {
          struct pollfd pfd;
          pfd.fd = inotify_fd;
          pfd.events = events;
          int poll_result = syscall_poll(&pfd, 1, timeout_ms);
          revents = pfd.revents;
          return poll_result;
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

        int inotify_fd = -1;
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

  watch_thread = thread_factory->create_thread([&]() { this->poll_watches(); });
}

template<typename SuperClassT>
fw::dm::OSFileSystem<SuperClassT>::~OSFileSystem()
{
  watches.clear();
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
      watch_thread->unsuspend();
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
    return;

  if (iter->second.remove_listener(listener))
    watches.erase(iter);

  if (watches.empty())
    watch_thread->suspend();
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

  short revents = 0;
  int event_count = inotify->poll(POLLIN, revents, -1);
  spdlog::debug("poll_watches: {}", event_count);

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
