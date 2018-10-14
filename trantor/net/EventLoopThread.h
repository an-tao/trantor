#pragma once

#include <trantor/net/EventLoop.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/SerialTaskQueue.h>
#include <mutex>
namespace trantor
{
class EventLoopThread : NonCopyable
{
  public:
    EventLoopThread();
    ~EventLoopThread();
    void run();
    //void stop();
    void wait();
    EventLoop *getLoop() { return loop_; }

  private:
    EventLoop *loop_;
    SerialTaskQueue loopQueue_;
    void loopFuncs();
    std::condition_variable cond_;
    std::mutex mutex_;
};
} // namespace trantor