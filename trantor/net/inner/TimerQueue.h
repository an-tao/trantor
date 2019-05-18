/**
 *
 *  TimerQueue.h
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

#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/net/callbacks.h>
#include <trantor/utils/Date.h>
#include "Timer.h"
#include <queue>
#include <memory>
#include <atomic>
#include <unordered_set>
namespace trantor
{
// class Timer;
class EventLoop;
class Channel;
typedef std::shared_ptr<Timer> TimerPtr;
struct comp
{
    bool operator()(const TimerPtr &x, const TimerPtr &y)
    {
        return *x > *y;
    }
};

class TimerQueue : NonCopyable
{
  public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();
    TimerId addTimer(const TimerCallback &cb,
                     const Date &when,
                     double interval);
    TimerId addTimer(TimerCallback &&cb, const Date &when, double interval);
    void addTimerInLoop(const TimerPtr &timer);
    void invalidateTimer(TimerId id);
#ifdef __linux__
    void reset();
#else
    int getTimeout() const;
    void processTimers();
#endif
  protected:
    EventLoop *_loop;
#ifdef __linux__
    int _timerfd;
    std::shared_ptr<Channel> _timerfdChannelPtr;
    void handleRead();
#endif
    std::priority_queue<TimerPtr, std::vector<TimerPtr>, comp> _timers;

    bool _callingExpiredTimers;
    bool insert(const TimerPtr &timePtr);
    std::vector<TimerPtr> getExpired();
    void reset(const std::vector<TimerPtr> &expired, const Date &now);
    std::vector<TimerPtr> getExpired(const Date &now);

  private:
    std::unordered_set<uint64_t> _timerIdSet;
};
};  // namespace trantor
