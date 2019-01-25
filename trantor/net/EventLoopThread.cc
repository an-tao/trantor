#include <trantor/net/EventLoopThread.h>
#include <trantor/utils/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

using namespace trantor;
EventLoopThread::EventLoopThread(const std::string &threadName)
    : _loop(NULL),
      _loopThreadName(threadName),
      _thread([=]() {
          loopFuncs();
      })
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (_loop == NULL)
    {
        _cond.wait(lock);
    }
}
EventLoopThread::~EventLoopThread()
{
    if (_loop) // not in 100% multiple thread security
    {
        _loop->quit();
    }
    if (_thread.joinable())
    {
        _thread.join();
    }
}
//void EventLoopThread::stop() {
//    if(_loop)
//        _loop->quit();
//}
void EventLoopThread::wait()
{
    _thread.join();
}
void EventLoopThread::loopFuncs()
{
#ifdef __linux__
    ::prctl(PR_SET_NAME, _loopThreadName.c_str());
#endif
    EventLoop loop;
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }
    auto f = _promiseForRun.get_future();
    (void)f.get();
    loop.loop();
    _loop = NULL;
}
void EventLoopThread::run()
{
    std::call_once(_once, [this]() {
        _promiseForRun.set_value(1);
    });
}
