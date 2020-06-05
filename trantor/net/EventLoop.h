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

#pragma once
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/Date.h>
#include <trantor/utils/LockFreeQueue.h>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>
#include <chrono>
#include <limits>

namespace trantor
{
class Poller;
class TimerQueue;
class Channel;
using ChannelList = std::vector<Channel *>;
using Func = std::function<void()>;
using TimerId = uint64_t;
enum
{
    InvalidTimerId = 0
};

class EventLoop : NonCopyable
{
  public:
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    };
#ifdef __linux__
    void resetTimerQueue();
#endif
    void resetAfterFork();
    bool isInLoopThread() const
    {
        return threadId_ == std::this_thread::get_id();
    };
    static EventLoop *getEventLoopOfCurrentThread();
    void updateChannel(Channel *chl);
    void removeChannel(Channel *chl);
    void runInLoop(const Func &f);
    void runInLoop(Func &&f);
    void queueInLoop(const Func &f);
    void queueInLoop(Func &&f);
    void wakeup();
    void wakeupRead();
    size_t index()
    {
        return index_;
    }
    void setIndex(size_t index)
    {
        index_ = index;
    }

    ///
    /// Runs callback at 'time'.
    /// Safe to call from other threads.
    ///
    TimerId runAt(const Date &time, const Func &cb);
    TimerId runAt(const Date &time, Func &&cb);
    ///
    /// Runs callback after @c delay seconds.
    /// Safe to call from other threads.
    ///
    TimerId runAfter(double delay, const Func &cb);
    TimerId runAfter(double delay, Func &&cb);

    /// Runs @param cb after @param delay
    /**
     * Users could use chrono literals to represent a time duration
     * For example:
     * @code
       runAfter(5s, task);
       runAfter(10min, task);
       @endcode
     */
    TimerId runAfter(const std::chrono::duration<long double> &delay,
                     const Func &cb)
    {
        return runAfter(delay.count(), cb);
    }
    TimerId runAfter(const std::chrono::duration<long double> &delay, Func &&cb)
    {
        return runAfter(delay.count(), std::move(cb));
    }
    ///
    /// Runs callback every @c interval seconds.
    /// Safe to call from other threads.
    ///
    TimerId runEvery(double interval, const Func &cb);
    TimerId runEvery(double interval, Func &&cb);

    /// Runs @param cb every @param interval time
    /**
     * Users could use chrono literals to represent a time duration
     * For example:
     * @code
       runEvery(5s, task);
       runEvery(10min, task);
       runEvery(0.1h, task);
       @endcode
     */
    TimerId runEvery(const std::chrono::duration<long double> &interval,
                     const Func &cb)
    {
        return runEvery(interval.count(), cb);
    }
    TimerId runEvery(const std::chrono::duration<long double> &interval,
                     Func &&cb)
    {
        return runEvery(interval.count(), std::move(cb));
    }
    // int getAioEventFd();
    // io_context_t getAioContext() {return ctx_;};
    void invalidateTimer(TimerId id);

    bool isRunning()
    {
        return looping_ && (!quit_);
    }
    bool isCallingFunctions()
    {
        return callingFuncs_;
    }
    /**
     * @brief Move the EventLoop to the current thread, this method must be
     * called before the loop is running.
     *
     */
    void moveToCurrentThread();

  private:
    void abortNotInLoopThread();
    bool looping_;
    std::thread::id threadId_;
    bool quit_;
    std::unique_ptr<Poller> poller_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    bool eventHandling_;
    MpscQueue<Func> funcs_;
    std::unique_ptr<TimerQueue> timerQueue_;
    bool callingFuncs_{false};
#ifdef __linux__
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannelPtr_;
#elif defined _WIN32
#else
    int wakeupFd_[2];
    std::unique_ptr<Channel> wakeupChannelPtr_;
#endif

    void doRunInLoopFuncs();
#ifdef _WIN32
    size_t index_{size_t(-1)};
#else
    size_t index_{std::numeric_limits<size_t>::max()};
#endif
    EventLoop **threadLocalLoopPtr_;
};

}  // namespace trantor
