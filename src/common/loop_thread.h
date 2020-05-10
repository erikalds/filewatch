#ifndef LOOP_THREAD_H
#define LOOP_THREAD_H

#include <functional>
#include <memory>

namespace threading
{
  class LoopThread
  {
  public:
    virtual ~LoopThread() = 0;

    virtual void join() = 0;
    virtual bool is_joinable() const = 0;

    virtual void suspend() = 0;
    virtual void unsuspend() = 0;
    virtual bool is_suspended() const = 0;

    virtual void stop() = 0;
    virtual bool is_stopped() const = 0;
  };

  class LoopThreadFactory
  {
  public:
    virtual ~LoopThreadFactory() = 0;

    virtual std::unique_ptr<LoopThread>
    create_thread(std::function<void()> iteration_fun,
                  bool create_suspended = false) = 0;
  };

  std::unique_ptr<LoopThreadFactory> create_thread_factory();

}  // namespace threading

#endif /* LOOP_THREAD_H */
