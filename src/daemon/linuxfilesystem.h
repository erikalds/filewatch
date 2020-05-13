#ifndef LINUXFILESYSTEM_H
#define LINUXFILESYSTEM_H

#ifdef __linux__

#  include "common/loop_thread.h"
#  include "defaultfilesystem.h"
#  include "details/inotify.h"
#  include "directoryeventlistener.h"

#  include <spdlog/spdlog.h>

#  include <functional>
#  include <map>
#  include <set>

struct inotify_event;

namespace fw
{
  namespace dm
  {
    namespace dtls
    {
      using GetDirEntryFun = std::function<std::optional<fs::DirectoryEntry>(std::string_view, std::string_view)>;

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
                   std::string filename,
                   const GetDirEntryFun& get_direntry,
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

    namespace dtls {

      std::optional<short>
      safe_poll_inotify(Inotify& inotify,
                        std::atomic<int>& currently_polling) noexcept;
      void process_inotify_events(Inotify& inotify,
                                  const std::map<std::string, dtls::Watch>& watches,
                                  const GetDirEntryFun& get_direntry) noexcept;
      void close_inotify(Inotify& inotify) noexcept;

    }  // namespace dtls
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
  close_inotify(*inotify);
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
    listener.notify(filewatch::DirectoryEvent::WATCHING_DIRECTORY, dirname, ".",
                    0);

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

template<typename SuperClassT>
void fw::dm::OSFileSystem<SuperClassT>::poll_watches()
{
  auto optional_revents = safe_poll_inotify(*inotify, currently_polling);
  if (!optional_revents)
    return;

  process_inotify_events(*inotify, watches,
                         [&](std::string_view containing_dir,
                             std::string_view filename)
                         {
                           return this->get_direntry(this->join(containing_dir,
                                                                filename));
                         });
}

#endif  // __linux__

#endif /* LINUXFILESYSTEM_H */
