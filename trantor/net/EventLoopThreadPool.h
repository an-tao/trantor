#pragma once

#include <trantor/net/EventLoopThread.h>
#include <vector>
namespace trantor
{
class EventLoopThreadPool : NonCopyable
{
  public:
    EventLoopThreadPool() = delete;
    EventLoopThreadPool(size_t threadNum);
    void start();
    //void stop();
    void wait();
    size_t getLoopNum() { return loopThreadVector_.size(); }
    EventLoop *getNextLoop();

  private:
    std::vector<EventLoopThread> loopThreadVector_;
    size_t loopIndex_;
};
} // namespace trantor