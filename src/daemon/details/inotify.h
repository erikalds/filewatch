#ifndef details_inotify_h
#define details_inotify_h

#ifdef __linux__

// #  include <limits.h>
#  include <poll.h>  // for nfds_t

#  include <spdlog/spdlog.h>

#  include <memory>

namespace fw
{
  namespace dm
  {
    namespace dtls
    {
      /// Handle the inotify subsystem on linux
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

        int add_watch(const char* pathname, uint32_t mask);
        int rm_watch(int wd);
        ssize_t read(void* buf, size_t count);
        void terminate_poll();
        int poll(short events, short& revents, int timeout_ms) noexcept;

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
        std::array<int, 2> pipe_fds = {0, 0};
      };

    }  // namespace dtls
  }  // namespace dm
}  // namespace fw

#endif  // __linux__

#endif  // details_inotify_h
