#include <trantor/utils/SerialTaskQueue.h>
#include <iostream>
namespace trantor
{
    SerialTaskQueue::SerialTaskQueue(const std::string &name)
    :queueName_(name),
     thread_(std::bind(&SerialTaskQueue::queueFunc,this))
    {
        if(name.empty())
        {
            queueName_="serail_task_queue_";
        }
        std::cout<<"constract SerialTaskQueue('"<<queueName_<<"')"<<std::endl;
    }
    SerialTaskQueue::~SerialTaskQueue() {
        stop_= true;
        taskCond_.notify_all();
        thread_.join();
        std::cout<<"unconstract SerialTaskQueue('"<<queueName_<<"')"<<std::endl;
    }
    void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task) {
        std::unique_lock<std::mutex> lock(taskMutex_);
        taskQueue_.push(task);
        taskCond_.notify_one();
    }
    void SerialTaskQueue::queueFunc() {
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
                    std::cout<<"got a new task!"<<std::endl;
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