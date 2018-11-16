#include <trantor/net/EventLoopThreadPool.h>
using namespace trantor;
EventLoopThreadPool::EventLoopThreadPool(size_t threadNum)
    : loopThreadVector_(threadNum),
      loopIndex_(0)
{
}
void EventLoopThreadPool::start()
{
    for (unsigned int i = 0; i < loopThreadVector_.size(); i++)
    {
        loopThreadVector_[i].run();
    }
}
//void EventLoopThreadPool::stop(){
//    for(unsigned int i=0;i<loopThreadVector_.size();i++)
//    {
//        loopThreadVector_[i].stop();
//    }
//}
void EventLoopThreadPool::wait()
{
    for (unsigned int i = 0; i < loopThreadVector_.size(); i++)
    {
        loopThreadVector_[i].wait();
    }
}
EventLoop *EventLoopThreadPool::getNextLoop()
{
    if (loopThreadVector_.size() > 0)
    {
        EventLoop *loop = loopThreadVector_[loopIndex_].getLoop();
        loopIndex_++;
        if (loopIndex_ >= loopThreadVector_.size())
            loopIndex_ = 0;
        return loop;
    }
    return NULL;
}
