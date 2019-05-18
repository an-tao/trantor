/**
 *
 *  EventLoopThread.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#include <trantor/net/EventLoopThread.h>
#include <trantor/utils/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

using namespace trantor;
EventLoopThread::EventLoopThread(const std::string &threadName)
    : _loop(nullptr),
      _loopThreadName(threadName),
      _thread([=]() { loopFuncs(); })
{
    auto f = _promiseForLoopPointer.get_future();
    _loop = f.get();
}
EventLoopThread::~EventLoopThread()
{
    run();
    if (_loop)
    {
        _loop->quit();
    }
    if (_thread.joinable())
    {
        _thread.join();
    }
}
// void EventLoopThread::stop() {
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
    loop.queueInLoop([=]() { _promiseForLoop.set_value(1); });
    _promiseForLoopPointer.set_value(&loop);
    auto f = _promiseForRun.get_future();
    (void)f.get();
    loop.loop();
    // LOG_DEBUG << "loop out";
    _loop = NULL;
}
void EventLoopThread::run()
{
    std::call_once(_once, [this]() {
        auto f = _promiseForLoop.get_future();
        _promiseForRun.set_value(1);
        // Make sure the event loop loops before returning.
        (void)f.get();
    });
}
