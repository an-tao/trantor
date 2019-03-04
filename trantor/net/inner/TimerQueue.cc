/**
 *
 *  TimerQueue.cc
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

#include <trantor/net/EventLoop.h>

#include "TimerQueue.h"
#include "Channel.h"
#ifdef __linux__
#include <sys/timerfd.h>
#endif
#include <string.h>
#include <iostream>
#include <unistd.h>

using namespace trantor;
#ifdef __linux__
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        std::cerr << "create timerfd failed!" << std::endl;
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(const Date &when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Date::date().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / 1000000);
    ts.tv_nsec = static_cast<long>(
        (microseconds % 1000000) * 1000);
    return ts;
}
void resetTimerfd(int timerfd, const Date &expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        //LOG_SYSERR << "timerfd_settime()";
    }
}
void readTimerfd(int timerfd, const Date &now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    //LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        // LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

void TimerQueue::handleRead()
{
    _loop->assertInLoopThread();
    const Date now = Date::date();
    readTimerfd(_timerfd, now);

    std::vector<TimerPtr> expired = getExpired(now);

    _callingExpiredTimers = true;
    //cancelingTimers_.clear();
    // safe to callback outside critical section
    for (auto const &timerPtr : expired)
    {
        if (_timerIdSet.find(timerPtr->id()) != _timerIdSet.end())
        {
            timerPtr->run();
        }
    }
    _callingExpiredTimers = false;

    reset(expired, now);
}
#else
int howMuchTimeFromNow(const Date &when)
{
    int64_t microSeconds = when.microSecondsSinceEpoch() - Date::date().microSecondsSinceEpoch();
    if (microSeconds < 1000)
    {
        microSeconds = 1000;
    }
    return static_cast<int>(microSeconds / 1000);
}
void TimerQueue::processTimers()
{
    _loop->assertInLoopThread();
    const Date now = Date::date();

    std::vector<TimerPtr> expired = getExpired(now);

    _callingExpiredTimers = true;
    //cancelingTimers_.clear();
    // safe to callback outside critical section
    for (auto const &timerPtr : expired)
    {
        if (_timerIdSet.find(timerPtr->id()) != _timerIdSet.end())
        {
            timerPtr->run();
        }
    }
    _callingExpiredTimers = false;

    reset(expired, now);
}
#endif
///////////////////////////////////////
TimerQueue::TimerQueue(EventLoop *loop)
    : _loop(loop),
#ifdef __linux__
      _timerfd(createTimerfd()),
      _timerfdChannelPtr(new Channel(loop, _timerfd)),
#endif
      _timers(),
      _callingExpiredTimers(false)
{
#ifdef __linux__
    _timerfdChannelPtr->setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    _timerfdChannelPtr->enableReading();
#endif
}
#ifdef __linux__
void TimerQueue::reset()
{
    _loop->runInLoop([this]() {
        _timerfdChannelPtr->disableAll();
        _timerfdChannelPtr->remove();
        close(_timerfd);
        _timerfd = createTimerfd();
        _timerfdChannelPtr = std::make_shared<Channel>(_loop, _timerfd);
        _timerfdChannelPtr->setReadCallback(std::bind(&TimerQueue::handleRead, this));
        // we are always reading the timerfd, we disarm it with timerfd_settime.
        _timerfdChannelPtr->enableReading();
        if (!_timers.empty())
        {
            const Date nextExpire = _timers.top()->when();
            resetTimerfd(_timerfd, nextExpire);
        }
    });
}
#endif
TimerQueue::~TimerQueue()
{
#ifdef __linux__
    auto chlPtr = _timerfdChannelPtr;
    auto fd = _timerfd;
    _loop->runInLoop([chlPtr, fd]() {
        chlPtr->disableAll();
        chlPtr->remove();
        ::close(fd);
    });
#endif
}

TimerId TimerQueue::addTimer(const TimerCallback &cb, const Date &when, double interval)
{

    std::shared_ptr<Timer> timerPtr = std::make_shared<Timer>(cb, when, interval);

    _loop->runInLoop([=]() {
        addTimerInLoop(timerPtr);
    });
    return timerPtr->id();
}
TimerId TimerQueue::addTimer(TimerCallback &&cb, const Date &when, double interval)
{

    std::shared_ptr<Timer> timerPtr = std::make_shared<Timer>(std::move(cb), when, interval);

    _loop->runInLoop([=]() {
        addTimerInLoop(timerPtr);
    });
    return timerPtr->id();
}
void TimerQueue::addTimerInLoop(const TimerPtr &timer)
{
    _loop->assertInLoopThread();
    _timerIdSet.insert(timer->id());
    if (insert(timer))
    {
        //the earliest timer changed
#ifdef __linux__
        resetTimerfd(_timerfd, timer->when());
#endif
    }
}

void TimerQueue::invalidateTimer(TimerId id)
{
    _loop->runInLoop([=]() {
        _timerIdSet.erase(id);
    });
}

bool TimerQueue::insert(const TimerPtr &timerPtr)
{
    _loop->assertInLoopThread();
    bool earliestChanged = false;
    if (_timers.size() == 0 || *timerPtr < *_timers.top())
    {
        earliestChanged = true;
    }
    _timers.push(timerPtr);
    //std::cout<<"after push new timer:"<<timerPtr->when().microSecondsSinceEpoch()/1000000<<std::endl;
    return earliestChanged;
}
#ifndef __linux__
int TimerQueue::getTimeout() const
{
    _loop->assertInLoopThread();
    if (_timers.empty())
    {
        return 10000;
    }
    else
    {
        return howMuchTimeFromNow(_timers.top()->when());
    }
}
#endif

std::vector<TimerPtr> TimerQueue::getExpired(const Date &now)
{
    std::vector<TimerPtr> expired;
    while (!_timers.empty())
    {
        if (_timers.top()->when() < now)
        {
            expired.push_back(_timers.top());
            _timers.pop();
        }
        else
            break;
    }
    return expired;
}
void TimerQueue::reset(const std::vector<TimerPtr> &expired, const Date &now)
{
    _loop->assertInLoopThread();
    for (auto const &timerPtr : expired)
    {
        if (timerPtr->isRepeat() &&
            _timerIdSet.find(timerPtr->id()) != _timerIdSet.end())
        {
            timerPtr->restart(now);
            insert(timerPtr);
        }
    }
#ifdef __linux__
    if (!_timers.empty())
    {
        const Date nextExpire = _timers.top()->when();
        resetTimerfd(_timerfd, nextExpire);
    }
#endif
}
