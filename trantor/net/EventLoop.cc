// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

// Taken from Muduo and modified
// Copyright 2016, Tao An.  All rights reserved.
// https://github.com/an-tao/trantor
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#include <trantor/net/EventLoop.h>
#include <trantor/utils/Logger.h>

#include "Poller.h"
#include "TimerQueue.h"
#include "Channel.h"

#include <thread>
#include <assert.h>
#include <poll.h>
#include <iostream>
#ifdef __linux__
#include <sys/eventfd.h>
#endif
#include <functional>
#include <unistd.h>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>

using namespace std;
namespace trantor
{
#ifdef __linux__
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        std::cout << "Failed in eventfd" << std::endl;
        abort();
    }

    return evtfd;
}
const int kPollTimeMs = 10000;
#endif
__thread EventLoop *t_loopInThisThread = 0;

EventLoop::EventLoop()
    : _looping(false),
      _threadId(std::this_thread::get_id()),
      _quit(false),
      _poller(Poller::newPoller(this)),
      _currentActiveChannel(NULL),
      _eventHandling(false),
      _timerQueue(new TimerQueue(this))
#ifdef __linux__
      ,
      _wakeupFd(createEventfd()),
      _wakeupChannelPtr(new Channel(this, _wakeupFd))
#endif
{
    assert(t_loopInThisThread == 0);
    t_loopInThisThread = this;
#ifdef __linux__
#else
    auto r = pipe(_wakeupFd);
    (void)r;
    assert(!r);
    fcntl(_wakeupFd[0], F_SETFL, O_NONBLOCK | O_CLOEXEC);
    fcntl(_wakeupFd[1], F_SETFL, O_NONBLOCK | O_CLOEXEC);
    _wakeupChannelPtr = std::unique_ptr<Channel>(new Channel(this, _wakeupFd[0]));

#endif
    _wakeupChannelPtr->setReadCallback(
        std::bind(&EventLoop::wakeupRead, this));
    _wakeupChannelPtr->enableReading();
}
#ifdef __linux__
void EventLoop::resetTimerQueue()
{
    assertInLoopThread();
    assert(!_looping);
    _timerQueue->reset();
}
#endif
void EventLoop::resetAfterFork()
{
    _poller->resetAfterFork();
}
EventLoop::~EventLoop()
{
    assert(!_looping);
    t_loopInThisThread = NULL;
#ifdef __linux__
    close(_wakeupFd);
#else
    close(_wakeupFd[0]);
    close(_wakeupFd[1]);
#endif
}
EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}
void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    _poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (_eventHandling)
    {
        assert(_currentActiveChannel == channel ||
               std::find(_activeChannels.begin(), _activeChannels.end(), channel) == _activeChannels.end());
    }
    _poller->removeChannel(channel);
}
void EventLoop::quit()
{
    _quit = true;
    // There is a chance that loop() just executes while(!_quit) and exits,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!isInLoopThread())
    {
        wakeup();
    }
}
void EventLoop::loop()
{
    assert(!_looping);
    assertInLoopThread();
    _looping = true;
    _quit = false;

    while (!_quit)
    {
        _activeChannels.clear();
#ifdef __linux__
        _poller->poll(kPollTimeMs, &_activeChannels);
#else
        _poller->poll(_timerQueue->getTimeout(), &_activeChannels);
        _timerQueue->processTimers();
#endif
        // TODO sort channel by priority
        //std::cout<<"after ->poll()"<<std::endl;
        _eventHandling = true;
        for (auto it = _activeChannels.begin();
             it != _activeChannels.end(); ++it)
        {
            _currentActiveChannel = *it;
            _currentActiveChannel->handleEvent();
        }
        _currentActiveChannel = NULL;
        _eventHandling = false;
        //std::cout << "looping" << endl;
        doRunInLoopFuncs();
    }
    _looping = false;
}
void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "It is forbidden to run loop on threads other than event-loop thread";
    exit(-1);
}
void EventLoop::runInLoop(const Func &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}
void EventLoop::runInLoop(Func &&cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}
void EventLoop::queueInLoop(const Func &cb)
{
    _funcs.enqueue(cb);
    if (!isInLoopThread() || !_looping)
    {
        wakeup();
    }
}
void EventLoop::queueInLoop(Func &&cb)
{
    _funcs.enqueue(std::move(cb));
    if (!isInLoopThread() || !_looping)
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Date &time, const Func &cb)
{
    return _timerQueue->addTimer(cb, time, 0);
}
TimerId EventLoop::runAt(const Date &time, Func &&cb)
{
    return _timerQueue->addTimer(std::move(cb), time, 0);
}
TimerId EventLoop::runAfter(double delay, const Func &cb)
{
    return runAt(Date::date().after(delay), cb);
}
TimerId EventLoop::runAfter(double delay, Func &&cb)
{
    return runAt(Date::date().after(delay), std::move(cb));
}
TimerId EventLoop::runEvery(double interval, const Func &cb)
{
    return _timerQueue->addTimer(cb, Date::date().after(interval), interval);
}
TimerId EventLoop::runEvery(double interval, Func &&cb)
{
    return _timerQueue->addTimer(std::move(cb), Date::date().after(interval), interval);
}
void EventLoop::invalidateTimer(TimerId id)
{
    if (isRunning() && _timerQueue)
        _timerQueue->invalidateTimer(id);
}
void EventLoop::doRunInLoopFuncs()
{
    _callingFuncs = true;
    {
        Func func;
        while (_funcs.dequeue(func))
        {
            func();
        }
    }
    _callingFuncs = false;
}
void EventLoop::wakeup()
{
    // if (!_looping)
    //     return;
    uint64_t tmp = 1;
#ifdef __linux__
    int ret = write(_wakeupFd, &tmp, sizeof(tmp));
#else
    int ret = write(_wakeupFd[1], &tmp, sizeof(tmp));
#endif
    if (ret < 0)
    {
        //LOG_SYSERR << "wakeup error";
    }
}
void EventLoop::wakeupRead()
{
    uint64_t tmp;
    ssize_t ret = 0;
#ifdef __linux__
    ret = read(_wakeupFd, &tmp, sizeof(tmp));
#else
    ret = read(_wakeupFd[0], &tmp, sizeof(tmp));
#endif
    if (ret < 0)
        LOG_SYSERR << "wakeup read error";
}
} // namespace trantor
