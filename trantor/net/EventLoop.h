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

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>
namespace trantor
{
class Poller;
class TimerQueue;
class Channel;
typedef std::vector<Channel *> ChannelList;
typedef std::function<void()> Func;
typedef uint64_t TimerId;
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
    ///
    /// Runs callback every @c interval seconds.
    /// Safe to call from other threads.
    ///
    TimerId runEvery(double interval, const Func &cb);
    TimerId runEvery(double interval, Func &&cb);
    //int getAioEventFd();
    //io_context_t getAioContext() {return ctx_;};
    void invalidateTimer(TimerId id);

    bool isRunning() { return looping_ && (!quit_); }

  private:
    void abortNotInLoopThread();
    bool looping_;
    const std::thread::id threadId_;
    bool quit_;
    std::unique_ptr<Poller> poller_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    bool eventHandling_;

    std::mutex funcsMutex_;

    std::vector<Func> funcs_;
    std::unique_ptr<TimerQueue> timerQueue_;
    bool callingFuncs_ = false;
#ifdef __linux__
    int wakeupFd_;
#else
    int wakeupFd_[2];
#endif
    std::unique_ptr<Channel> wakeupChannelPtr_;

    void doRunInLoopFuncs();
};
} // namespace trantor
