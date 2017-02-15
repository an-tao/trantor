#include <trantor/utils/SerialTaskQueue.h>
#include <trantor/utils/Logger.h>
#include <sys/prctl.h>

namespace trantor
{
    SerialTaskQueue::SerialTaskQueue(const std::string &name)
    :queueName_(name),
     stop_(false),
     thread_(std::bind(&SerialTaskQueue::queueFunc,this)),
     isRunTask_(false)
    {
        if(name.empty())
        {
            queueName_="SerailTaskQueue";
        }
        LOG_TRACE<<"construct SerialTaskQueue('"<<queueName_<<"')";
    }
    void SerialTaskQueue::stop() {
        stop_= true;
        taskCond_.notify_all();
        thread_.join();
    }
    SerialTaskQueue::~SerialTaskQueue() {
        if(!stop_)
            stop();
        LOG_TRACE<<"unconstruct SerialTaskQueue('"<<queueName_<<"')";
    }
    void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task)
    {
        LOG_TRACE<<"copy task into queue";
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskQueue_.push(task);
        taskCond_.notify_one();
    }
    void SerialTaskQueue::runTaskInQueue(std::function<void ()> &&task)
    {
        LOG_TRACE<<"move task into queue";
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskQueue_.push(std::move(task));
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
            isRunTask_=true;
            r();
            isRunTask_=false;
        }
    }
    void SerialTaskQueue::waitAllTasksFinished()
    {
        syncTaskInQueue([](){

        });
    }
    size_t SerialTaskQueue::getTaskCount()
    {
        std::lock_guard<std::mutex> guard(taskMutex_);
        return taskQueue_.size();
    }
};