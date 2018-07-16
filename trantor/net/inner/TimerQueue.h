#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/net/callbacks.h>
#include <trantor/utils/Date.h>
#include "Timer.h"
#include <queue>
#include <memory>
namespace trantor
{
    //class Timer;
    class EventLoop;
    class Channel;
    typedef std::shared_ptr<Timer> TimerPtr;
    struct comp{
        bool operator ()(const TimerPtr &x,const TimerPtr &y)
        {
            return *x > *y;
        }
    };

    class TimerQueue:NonCopyable
    {
    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();
        void addTimer(const TimerCallback &cb,const Date &when,double interval);
        void addTimer(TimerCallback &&cb,const Date &when,double interval);
        void addTimerInLoop(const TimerPtr &timer);

#ifndef __linux__
        int getTimeout() const;
        void processTimers();
#endif
    protected:

        EventLoop* loop_;
#ifdef __linux__
        const int timerfd_;
        std::unique_ptr<Channel> timerfdChannelPtr_;
        void handleRead();
#endif
        std::priority_queue<TimerPtr,std::vector<TimerPtr>,comp> timers_;

        bool callingExpiredTimers_;
        bool insert(const TimerPtr &timePtr);
        std::vector<TimerPtr> getExpired();
        void reset(const std::vector<TimerPtr>& expired,const Date &now);
        std::vector<TimerPtr> getExpired(const Date &now);
    };
};