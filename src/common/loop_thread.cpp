#include "loop_thread.h"

#include <atomic>
#include <condition_variable>
#include <thread>

namespace threading
{
  LoopThread::~LoopThread() = default;

  namespace
  {
    class DefaultLoopThread : public LoopThread
    {
    public:
      explicit DefaultLoopThread(std::function<void()> iter_fun) :
        iteration_function(std::move(iter_fun)), stopped(false),
        suspended(true), thread{[&]() { this->run(); }}
      {
      }
      DefaultLoopThread(const DefaultLoopThread&) = delete;
      DefaultLoopThread& operator=(const DefaultLoopThread&) = delete;
      DefaultLoopThread(DefaultLoopThread&&) = delete;
      DefaultLoopThread& operator=(DefaultLoopThread&&) = delete;
      ~DefaultLoopThread() override;

      void join() override { nv_join(); }
      bool is_joinable() const override { return nv_is_joinable(); }

      void suspend() override;
      void unsuspend() override { nv_unsuspend(); }
      bool is_suspended() const override { return nv_is_suspended(); }

      void stop() override { nv_stop(); }
      bool is_stopped() const override { return nv_is_stopped(); }

    private:
      void nv_join();
      bool nv_is_joinable() const { return thread.joinable(); }
      bool nv_is_suspended() const;
      bool nv_is_stopped() const { return !thread.joinable(); }
      void nv_unsuspend();
      void nv_stop() { stopped = true; }
      void run();

      std::function<void()> iteration_function;
      std::atomic<bool> stopped;
      bool suspended;
      mutable std::mutex suspended_mutex;
      std::condition_variable suspended_condition;
      std::thread thread;
    };

    DefaultLoopThread::~DefaultLoopThread()
    {
      if (!nv_is_stopped())
      {
        if (nv_is_suspended())
        {
          nv_unsuspend();
        }
        nv_stop();
      }
      if (nv_is_joinable())
      {
        nv_join();
      }
    }

    void DefaultLoopThread::suspend()
    {
      std::lock_guard<std::mutex> sentry(suspended_mutex);
      suspended = true;
    }

    void DefaultLoopThread::nv_unsuspend()
    {
      std::lock_guard<std::mutex> sentry(suspended_mutex);
      suspended = false;
      suspended_condition.notify_one();
    }

    void DefaultLoopThread::nv_join()
    {
      if (nv_is_suspended())
      {
        nv_unsuspend();
      }
      thread.join();
    }

    bool DefaultLoopThread::nv_is_suspended() const
    {
      std::lock_guard<std::mutex> sentry(suspended_mutex);
      return suspended;
    }

    void DefaultLoopThread::run()
    {
      while (!stopped)
      {
        {
          std::unique_lock<std::mutex> sentry(suspended_mutex);
          suspended_condition.wait(sentry,
                                   [&]() { return !suspended || stopped; });
          if (stopped)
          {
            break;
          }
        }

        iteration_function();
      }
    }

  }  // anonymous namespace

  LoopThreadFactory::~LoopThreadFactory() = default;

  namespace
  {
    class DefaultLoopThreadFactory : public LoopThreadFactory
    {
    public:
      std::unique_ptr<LoopThread>
      create_thread(std::function<void()> iteration_fun,
                    bool create_suspended) override
      {
        auto thread = std::make_unique<DefaultLoopThread>(iteration_fun);
        if (!create_suspended)
        {
          thread->unsuspend();
        }
        return thread;
      }
    };

  }  // anonymous namespace

  std::unique_ptr<LoopThreadFactory> create_thread_factory()
  {
    return std::make_unique<DefaultLoopThreadFactory>();
  }

}  // namespace threading
