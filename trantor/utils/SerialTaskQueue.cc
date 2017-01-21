#include <trantor/utils/SerialTaskQueue.h>
#include <trantor/utils/Logger.h>
#include <sys/prctl.h>
#include <iostream>
namespace trantor
{
    SerialTaskQueue::SerialTaskQueue(const std::string &name)
    :queueName_(name),
     thread_(std::bind(&SerialTaskQueue::queueFunc,this))
    {
        if(name.empty())
        {
            queueName_="SerailTaskQueue";
        }
        LOG_TRACE<<"constract SerialTaskQueue('"<<queueName_<<"')";
    }
    SerialTaskQueue::~SerialTaskQueue() {
        stop_= true;
        taskCond_.notify_all();
        thread_.join();
        LOG_TRACE<<"unconstract SerialTaskQueue('"<<queueName_<<"')";
    }
    void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task) {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskQueue_.push(task);
        taskCond_.notify_one();
    }
    void SerialTaskQueue::queueFunc() {
        ::prctl(PR_SET_NAME,queueName_.c_str());
        while(!stop_)
        {
            std::function<void ()> r;
            {
                std::unique_lock<std::mutex> lock(taskMutex_);
                while (!stop_ && taskQueue_.size() == 0) {
                    taskCond_.wait(lock);
                }
                if(taskQueue_.size()>0)
                {
                    LOG_TRACE<<"got a new task!";
                    r=std::move(taskQueue_.front());
                    taskQueue_.pop();
                }
                else
                    continue;
            }
            r();
        }
    }
    void SerialTaskQueue::waitAllTasksFinished()
    {
        syncTaskInQueue([](){

        });
    }
};