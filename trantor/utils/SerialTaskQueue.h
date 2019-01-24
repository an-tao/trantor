#pragma once

#include "TaskQueue.h"
#include <trantor/net/EventLoopThread.h>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
namespace trantor
{
class SerialTaskQueue : public TaskQueue
{
  public:
    virtual void runTaskInQueue(const std::function<void()> &task);
    virtual void runTaskInQueue(std::function<void()> &&task);

    virtual std::string getName() const { return _queueName; };
    void waitAllTasksFinished();
    SerialTaskQueue() = delete;
    explicit SerialTaskQueue(const std::string &name = std::string());
    ~SerialTaskQueue();
    bool isRuningTask() { return _loopThread.getLoop() ? _loopThread.getLoop()->isCallingFunctions() : false; }
    size_t getTaskCount();
    void stop();

  protected:
    std::string _queueName;
    EventLoopThread _loopThread;
    bool _stop = false;
};
}; // namespace trantor
