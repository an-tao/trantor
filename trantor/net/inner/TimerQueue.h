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
        TimerId addTimer(const TimerCallback &cb,const Date &when,double interval);
        TimerId addTimer(TimerCallback &&cb,const Date &when,double interval);
        void addTimerInLoop(const TimerPtr &timer);
        void invalidateTimer(TimerId id);
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

    private:
        std::atomic<uint64_t> _timerIdGenarator=ATOMIC_VAR_INIT(0);
        std::unordered_set<uint64_t> _timerIdSet;
    };
};