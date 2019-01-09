#pragma once

#include "TaskQueue.h"
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
    bool isRuningTask() { return _isRuningTask; }
    size_t getTaskCount();
    void stop();

  protected:
    std::string _queueName;
    std::queue<std::function<void()>> _taskQueue;
    std::mutex _taskMutex;
    std::condition_variable _taskCond;
    std::atomic_bool _stop;
    void queueFunc();
    std::thread _thread;
    std::atomic_bool _isRuningTask;
};
}; // namespace trantor
