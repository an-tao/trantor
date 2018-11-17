#include <trantor/net/EventLoopThread.h>
#include <trantor/utils/Logger.h>
#include <trantor/utils/SerialTaskQueue.h>

using namespace trantor;
EventLoopThread::EventLoopThread()
    : loop_(NULL),
      loopQueue_("EventLoopThread")
{
}
EventLoopThread::~EventLoopThread()
{
    if (loop_) // not in 100% multiple thread security
    {
        loop_->quit();
    }
}
//void EventLoopThread::stop() {
//    if(loop_)
//        loop_->quit();
//}
void EventLoopThread::wait()
{
    loopQueue_.waitAllTasksFinished();
}
void EventLoopThread::loopFuncs()
{
    EventLoop loop;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    loop_ = NULL;
}
void EventLoopThread::run()
{
    loopQueue_.runTaskInQueue(std::bind(&EventLoopThread::loopFuncs, this));
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == NULL)
    {
        cond_.wait(lock);
    }
}
