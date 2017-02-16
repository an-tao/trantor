#pragma once

#include <trantor/utils/TaskQueue.h>
#include <list>
#include <memory>
#include <vector>
#include <queue>
namespace trantor
{
    class ConcurrentTaskQueue:public TaskQueue
    {
    public:
        ConcurrentTaskQueue(size_t threadNum,const std::string &name);
        virtual  void runTaskInQueue(const std::function<void ()> &task);
        virtual  void runTaskInQueue(std::function<void ()> &&task)=0;

        virtual  std::string getName() const {return queueName_;};
        size_t getTaskCount();
    private:

        size_t queueCount_;
        std::string queueName_;

        std::queue<std::function<void ()> > taskQueue_;
        std::vector<std::thread> threads_;

        std::mutex taskMutex_;
        std::condition_variable taskCond_;
        std::atomic_bool stop_;
        void queueFunc(int queueNum);
        std::thread thread_;


    };
}