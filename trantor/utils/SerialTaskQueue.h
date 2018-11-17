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

    virtual std::string getName() const { return queueName_; };
    void waitAllTasksFinished();
    SerialTaskQueue() = delete;
    SerialTaskQueue(const std::string &name = std::string());
    ~SerialTaskQueue();
    bool isRunTask() { return isRunTask_; }
    size_t getTaskCount();
    void stop();

  protected:
    std::string queueName_;
    std::queue<std::function<void()>> taskQueue_;
    std::mutex taskMutex_;
    std::condition_variable taskCond_;
    std::atomic_bool stop_;
    void queueFunc();
    std::thread thread_;
    std::atomic_bool isRunTask_;
};
}; // namespace trantor
