#include <trantor/net/EventLoopThread.h>
#include <trantor/utils/Logger.h>
#include <trantor/utils/SerialTaskQueue.h>

using namespace trantor;
EventLoopThread::EventLoopThread(const std::string &threadName)
    : _loop(NULL),
      _loopQueue(threadName)
{
}
EventLoopThread::~EventLoopThread()
{
    if (_loop) // not in 100% multiple thread security
    {
        _loop->quit();
    }
}
//void EventLoopThread::stop() {
//    if(_loop)
//        _loop->quit();
//}
void EventLoopThread::wait()
{
    _loopQueue.waitAllTasksFinished();
}
void EventLoopThread::loopFuncs()
{
    EventLoop loop;
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }
    loop.loop();
    _loop = NULL;
}
void EventLoopThread::run()
{
    _loopQueue.runTaskInQueue(std::bind(&EventLoopThread::loopFuncs, this));
    std::unique_lock<std::mutex> lock(_mutex);
    while (_loop == NULL)
    {
        _cond.wait(lock);
    }
}
