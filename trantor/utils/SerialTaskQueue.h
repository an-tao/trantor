#pragma once

#include "TaskQueue.h"
#include <string>
#include <queue>
#include <mutex>
namespace trantor
{
    class SerialTaskQueue:public TaskQueue
    {
    public:
        virtual  void runTaskInQueue(const std::function<void ()> &task);
        virtual  std::string getName() const {return queueName_;};
        void waitAllTasksFinished();
        SerialTaskQueue()=delete;
        SerialTaskQueue(const std::string &name=std::string());
        ~SerialTaskQueue();
    protected:
        std::string queueName_;
        std::queue<std::function<void ()> > taskQueue_;
        std::mutex taskMutex_;
        std::condition_variable taskCond_;
        bool stop_=false;
        void queueFunc();
        std::thread thread_;
    };
};