#include "common/loop_thread.h"
#include "daemon/directoryeventlistener.h"
#include "daemon/linuxfilesystem.h"
#include "dummyfilesystem.h"

#include <catch2/catch.hpp>

#ifdef __linux__

#  include <sys/inotify.h>

#  include <cstring>

namespace
{
  std::size_t enforce_4byte_alignment(std::size_t size)
  {
    return static_cast<uint32_t>((1 + (size / 4)) * 4);
  }


  class InotifyDummy : public fw::dm::dtls::Inotify
  {
  public:
    InotifyDummy() = default;
    InotifyDummy(const InotifyDummy&) = delete;
    InotifyDummy& operator=(const InotifyDummy&) = delete;
    InotifyDummy(InotifyDummy&&) = delete;
    InotifyDummy& operator=(InotifyDummy&&) = delete;
    ~InotifyDummy() override
    {
      CHECK(closed_fd == init_fd);
      for (auto mem : waiting_events)
      {
        std::free(mem.first);  // NOLINT
      }
    }

    int init_fd = 42;
    int init_count = 0;
    int closed_fd = -1;
    int add_watch_error = 0;

    int syscall_inotify_init() override
    {
      ++init_count;
      return init_fd;
    }

    int syscall_close(int fd) override
    {
      CHECK(fd == init_fd);
      closed_fd = fd;
      return 0;
    }

    struct Watch
    {
      std::string pathname;
      uint32_t mask;
      bool active;
    };

    std::vector<Watch> watches;

    int syscall_inotify_add_watch(int fd, const char* pathname,
                                  uint32_t mask) override
    {
      CHECK(fd == init_fd);
      if (add_watch_error != 0)
      {
        errno = add_watch_error;
        return -1;
      }
      watches.push_back(Watch{pathname, mask, true});
      return static_cast<int>(watches.size()) - 1;
    }

    int syscall_inotify_rm_watch(int fd, int wd) override
    {
      CHECK(fd == init_fd);
      REQUIRE(wd < watches.size());
      watches[static_cast<std::size_t>(wd)].active = false;
      return 0;
    }

    ssize_t syscall_read(int fd, void* buf, size_t count) override
    {
      if (fd != init_fd)
      {
        return 0;
      }

      if (event_pos == waiting_events.size())
      {
        return 0;
      }

      size_t bytes_in_buf = 0;
      char* cbuf = static_cast<char*>(buf);
      while (event_pos < waiting_events.size())
      {
        if (waiting_events[event_pos].second > count)
        {
          break;
        }

        auto event_struct_size = waiting_events[event_pos].second;
        count -= event_struct_size;
        std::memcpy(cbuf, waiting_events[event_pos].first, event_struct_size);
        // NOLINTNEXTLINE (pointer arithmetic)
        cbuf += enforce_4byte_alignment(event_struct_size);
        bytes_in_buf += event_struct_size;
        ++event_pos;
      }
      if (bytes_in_buf > 0)
      {
        assert(bytes_in_buf
               <= static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
        return static_cast<ssize_t>(bytes_in_buf);
      }

      errno = EINVAL;
      return -1;
    }

    int syscall_poll(struct pollfd* fds, nfds_t nfds, int timeout_ms) override
    {
      int fd_count_with_events = 0;
      for (nfds_t i = 0; i < nfds; ++i)
      {
        if (fds[i].fd == init_fd)  // NOLINT pointer arithmetic
        {
          if ((fds[i].events & POLLIN) != 0)  // NOLINT pointer arithmetic
          {
            if (waiting_events.size() > event_pos)
            {
              fds[i].revents |= POLLIN;  // NOLINT pointer arithmetic
              ++fd_count_with_events;
            }
          }
        }
      }
      if (timeout_ms == -1 && fd_count_with_events == 0)
      {
        // emulate error when timeout set to infinity
        errno = EINTR;  // A signal occurred before any requested event
        return -1;
      }

      return fd_count_with_events;
    }

    void file_added(std::string_view containing_dir, std::string_view filename)
    {
      spdlog::debug("file added: {} in {}\n", filename, containing_dir);
      push_event(containing_dir, filename, IN_CREATE);
    }

    void file_removed(std::string_view containing_dir,
                      std::string_view filename)
    {
      spdlog::debug("file removed: {} in {}\n", filename, containing_dir);
      push_event(containing_dir, filename, IN_DELETE);
    }

    void push_event(std::string_view containing_dir, std::string_view filename,
                    uint32_t mask)
    {
      static_assert(NAME_MAX < 0xFFFFFFFF);
      REQUIRE(filename.size() < NAME_MAX);
      const auto structsize = sizeof(inotify_event) + filename.size();
      for (std::size_t i = 0; i < watches.size(); ++i)
      {
        if (watches[i].pathname == containing_dir)
        {
          void* eventmem = std::malloc(structsize);  // NOLINT
          REQUIRE(eventmem != nullptr);
          inotify_event* event = new (eventmem) inotify_event;  // NOLINT
          event->wd = static_cast<int>(i);
          event->mask = mask;
          event->cookie = 0;
          event->len = static_cast<uint32_t>(filename.size());
          // NOLINTNEXTLINE
          std::strncpy(event->name, filename.data(), filename.size());
          waiting_events.emplace_back(std::make_pair(eventmem, structsize));
        }
      }
    }

    std::vector<std::pair<void*, size_t>> waiting_events;
    std::size_t event_pos = 0;
  };

  class DummyLoopThread;

  class DummyThreads
  {
  public:
    DummyThreads() = default;

    void run_once();

    void add_thread(DummyLoopThread& th) { threads.insert(&th); }
    void remove_thread(DummyLoopThread& th) { threads.erase(&th); }

  private:
    std::set<DummyLoopThread*> threads;
  };

  class DummyLoopThread : public threading::LoopThread
  {
  public:
    DummyLoopThread(DummyThreads& threads_, std::function<void()> iter_fun) :
      threads(threads_), iteration_fun(std::move(iter_fun)), suspended(true),
      stopped(false), joined(false)
    {
      threads.add_thread(*this);
    }
    DummyLoopThread(const DummyLoopThread&) = delete;
    DummyLoopThread& operator=(const DummyLoopThread&) = delete;
    DummyLoopThread(DummyLoopThread&&) = delete;
    DummyLoopThread& operator=(DummyLoopThread&&) = delete;
    ~DummyLoopThread() override { threads.remove_thread(*this); }

    void join() override { joined = true; }
    [[nodiscard]] bool is_joinable() const override { return !joined; }
    void suspend() override { suspended = true; }
    void unsuspend() override { suspended = false; }
    [[nodiscard]] bool is_suspended() const override { return suspended; }
    void stop() override { stopped = true; }
    [[nodiscard]] bool is_stopped() const override { return stopped; }

    void run_one_iteration()
    {
      if (!stopped || !suspended)
      {
        iteration_fun();
      }
    }

  private:
    DummyThreads& threads;
    std::function<void()> iteration_fun;
    bool suspended, stopped, joined;
  };

  void DummyThreads::run_once()
  {
    for (auto* thread : threads)
    {
      thread->run_one_iteration();
    }
  }

  class DummyLoopThreadFactory : public threading::LoopThreadFactory
  {
  public:
    DummyLoopThreadFactory() : threads(my_threads) {}
    explicit DummyLoopThreadFactory(DummyThreads& threads_) : threads(threads_)
    {
    }

    std::unique_ptr<threading::LoopThread>
    create_thread(std::function<void()> iteration_fun,
                  bool create_suspended = false) override
    {
      auto t = std::make_unique<DummyLoopThread>(threads, iteration_fun);
      if (!create_suspended)
      {
        t->unsuspend();
      }
      return t;
    }

  private:
    DummyThreads my_threads;
    DummyThreads& threads;
  };

}  // anonymous namespace

TEST_CASE("inotify init and dtor", "[LinuxFileSystem]")
{
  auto ptr = fw::dm::dtls::Inotify::create<InotifyDummy>();
  auto* inotify = dynamic_cast<InotifyDummy*>(ptr.get());
  fw::dm::OSFileSystem<DummyFileSystem> fs(
    "/home/user/rootdir", std::move(ptr),
    std::make_unique<DummyLoopThreadFactory>());
  SECTION("init is called") { CHECK(inotify->init_count == 1); }

  // CHECK's in InotifyDummy dtor and syscall_close() verifies dtor behaviour
}

namespace
{
  class LoggingDirectoryEventListener : public fw::dm::DirectoryEventListener
  {
  public:
    struct Event
    {
      filewatch::DirectoryEvent::Event event;
      std::string containing_dir;
      std::string dir_name;
      uint64_t mtime;
    };

    std::vector<Event> events;

    void notify(filewatch::DirectoryEvent::Event event,
                std::string_view containing_dir,
                std::string_view dir_name,
                uint64_t mtime) override
    {
      spdlog::debug("LoggingDirectoryEventListener::notify(x, {}, {} {})\n",
                    containing_dir, dir_name, mtime);
      events.push_back(Event{event, std::string(containing_dir),
                             std::string(dir_name), mtime});
    }
  };

}  // anonymous namespace

TEST_CASE("add/remove DirectoryEventListener", "[LinuxFileSystem]")
{
  auto ptr = fw::dm::dtls::Inotify::create<InotifyDummy>();
  auto* inotify = dynamic_cast<InotifyDummy*>(ptr.get());
  fw::dm::OSFileSystem<DummyFileSystem> fs(
    "/home/user/rootdir", std::move(ptr),
    std::make_unique<DummyLoopThreadFactory>());
  LoggingDirectoryEventListener listener;
  SECTION("add/remove")
  {
    fs.watch("/my dir", listener);
    REQUIRE(inotify->watches.size() == 1);
    CHECK(inotify->watches[0].pathname == "/home/user/rootdir/my dir");
    fs.watch("/other/dir", listener);
    REQUIRE(inotify->watches.size() == 2);
    CHECK(inotify->watches[1].pathname == "/home/user/rootdir/other/dir");
    fs.stop_watching("/my dir", listener);
    CHECK(inotify->watches[0].active == false);
    fs.stop_watching("/other/dir", listener);
    CHECK(inotify->watches[1].active == false);
  }
  SECTION("several listeners to same dir gives only one watch")
  {
    fs.watch("/some dir", listener);
    REQUIRE(inotify->watches.size() == 1);
    LoggingDirectoryEventListener listener2;
    fs.watch("/some dir", listener2);
    CHECK(inotify->watches.size() == 1);
    CHECK(inotify->watches[0].active == true);
    fs.stop_watching("/some dir", listener);
    CHECK(inotify->watches[0].active == true);
    fs.stop_watching("/some dir", listener2);
    CHECK(inotify->watches[0].active == false);
  }
  SECTION("mask bits")
  {
    fs.watch("/my dir", listener);
    REQUIRE(inotify->watches.size() == 1);
    CHECK((inotify->watches[0].mask & IN_CLOSE_WRITE) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_CREATE) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_DELETE) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_DELETE_SELF) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_MOVE_SELF) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_MOVED_FROM) != 0);  // NOLINT
    CHECK((inotify->watches[0].mask & IN_MOVED_TO) != 0);  // NOLINT
  }
  SECTION("error codes")
  {
    inotify->add_watch_error = EACCES;
    CHECK_THROWS_WITH(fs.watch("/my dir", listener),
                      "/my dir error: Read access to the given file is not "
                      "permitted [EACCES].");

    inotify->add_watch_error = EBADF;
    CHECK_THROWS_WITH(
      fs.watch("/your dir", listener),
      "/your dir error: The given file descriptor is not valid [EBADF].");

    inotify->add_watch_error = EFAULT;
    CHECK_THROWS_WITH(fs.watch("/dir2", listener),
                      "/dir2 error: pathname points outside of the process's "
                      "accessible address space [EFAULT].");

    inotify->add_watch_error = EINVAL;
    CHECK_THROWS_WITH(
      fs.watch("/dir3", listener),
      "/dir3 error: The given event mask contains no valid events; or mask "
      "contains both IN_MASK_ADD and IN_MASK_CREATE; or fd is not an inotify "
      "file descriptor [EINVAL].");

    inotify->add_watch_error = ENAMETOOLONG;
    CHECK_THROWS_WITH(fs.watch("/dir4", listener),
                      "/dir4 error: pathname is too long [ENAMETOOLONG].");

    inotify->add_watch_error = ENOENT;
    CHECK_THROWS_WITH(fs.watch("/dir5", listener),
                      "/dir5 error: A directory component in pathname does not "
                      "exist or is a dangling symbolic link [ENOENT].");

    inotify->add_watch_error = ENOMEM;
    CHECK_THROWS_WITH(
      fs.watch("/dir6", listener),
      "/dir6 error: Insufficient kernel memory was available [ENOMEM].");

    inotify->add_watch_error = ENOSPC;
    CHECK_THROWS_WITH(
      fs.watch("/dir7", listener),
      "/dir7 error: The user limit on the total number of inotify watches was "
      "reached or the kernel failed to allocate a needed resource [ENOSPC].");

    inotify->add_watch_error = ENOTDIR;
    CHECK_THROWS_WITH(fs.watch("/dir8", listener),
                      "/dir8 error: mask contains IN_ONLYDIR and pathname is "
                      "not a directory [ENOTDIR].");

    inotify->add_watch_error = EEXIST;
    CHECK_THROWS_WITH(
      fs.watch("/dir9", listener),
      "/dir9 error: mask contains IN_MASK_CREATE and pathname refers to a file "
      "already being watched by the same fd [EEXIST].");

    inotify->add_watch_error = 244;
    CHECK_THROWS_WITH(fs.watch("/dir10", listener),
                      "/dir10 error: Unknown error code [244].");
  }
}

TEST_CASE("notify DirectoryEventListener", "[LinuxFileSystem]")
{
  auto ptr = fw::dm::dtls::Inotify::create<InotifyDummy>();
  auto* inotify = dynamic_cast<InotifyDummy*>(ptr.get());
  DummyThreads threads;
  fw::dm::OSFileSystem<DummyFileSystem> fs(
    "/home/user/rootdir", std::move(ptr),
    std::make_unique<DummyLoopThreadFactory>(threads));
  LoggingDirectoryEventListener listener;
  fs.add_dir("/", "dir", 12356780);
  fs.add_file("/dir", "filename", 12356789);
  fs.watch("/dir", listener);
  SECTION("add file")
  {
    inotify->file_added("/home/user/rootdir/dir", "filename");
    threads.run_once();
    REQUIRE(listener.events.size() == 1);
    CHECK(listener.events[0].event == filewatch::DirectoryEvent::FILE_ADDED);
    CHECK(listener.events[0].containing_dir == "/dir");
    CHECK(listener.events[0].dir_name == "filename");
    CHECK(listener.events[0].mtime == 12356789);
  }
  SECTION("remove file")
  {
    inotify->file_removed("/home/user/rootdir/dir", "filename");
    threads.run_once();
    REQUIRE(listener.events.size() == 1);
    CHECK(listener.events[0].event == filewatch::DirectoryEvent::FILE_REMOVED);
    CHECK(listener.events[0].containing_dir == "/dir");
    CHECK(listener.events[0].dir_name == "filename");
    CHECK(listener.events[0].mtime == 12356789);
  }
}

#endif  // __linux__
