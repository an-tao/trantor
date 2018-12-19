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
    : looping_(false),
      threadId_(std::this_thread::get_id()),
      quit_(false),
      poller_(Poller::newPoller(this)),
      currentActiveChannel_(NULL),
      eventHandling_(false),
      timerQueue_(new TimerQueue(this))
#ifdef __linux__
      ,
      wakeupFd_(createEventfd()),
      wakeupChannelPtr_(new Channel(this, wakeupFd_))
#endif
{
    assert(t_loopInThisThread == 0);
    t_loopInThisThread = this;
#ifdef __linux__
#else
    auto r = pipe(wakeupFd_);
    (void)r;
    assert(!r);

    wakeupChannelPtr_ = std::unique_ptr<Channel>(new Channel(this, wakeupFd_[0]));

#endif
    wakeupChannelPtr_->setReadCallback(
        std::bind(&EventLoop::wakeupRead, this));
    wakeupChannelPtr_->enableReading();
}
#ifdef __linux__
void EventLoop::resetTimerQueue()
{
    assertInLoopThread();
    assert(!looping_);
    timerQueue_->reset();
}
#endif
EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInThisThread = NULL;
#ifdef __linux__
    close(wakeupFd_);
#else
    close(wakeupFd_[0]);
    close(wakeupFd_[1]);
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
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}
void EventLoop::quit()
{
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exits,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!isInLoopThread())
    {
        wakeup();
    }
}
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
#ifdef __linux__
        poller_->poll(kPollTimeMs, &activeChannels_);
#else
        poller_->poll(timerQueue_->getTimeout(), &activeChannels_);
        timerQueue_->processTimers();
#endif
        // TODO sort channel by priority
        //std::cout<<"after ->poll()"<<std::endl;
        eventHandling_ = true;
        for (auto it = activeChannels_.begin();
             it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        //std::cout << "looping" << endl;
        doRunInLoopFuncs();
    }
    looping_ = false;
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
    {
        std::lock_guard<std::mutex> lock(funcsMutex_);
        funcs_.push_back(cb);
    }

    if (!isInLoopThread() || callingFuncs_ || !looping_)
    {
        wakeup();
    }
}
void EventLoop::queueInLoop(Func &&cb)
{
    {
        std::lock_guard<std::mutex> lock(funcsMutex_);
        funcs_.push_back(std::move(cb));
    }

    if (!isInLoopThread() || callingFuncs_ || !looping_)
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Date &time, const Func &cb)
{
    return timerQueue_->addTimer(cb, time, 0);
}
TimerId EventLoop::runAt(const Date &time, Func &&cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0);
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
    return timerQueue_->addTimer(cb, Date::date().after(interval), interval);
}
TimerId EventLoop::runEvery(double interval, Func &&cb)
{
    return timerQueue_->addTimer(std::move(cb), Date::date().after(interval), interval);
}
void EventLoop::invalidateTimer(TimerId id)
{
    if (timerQueue_)
        timerQueue_->invalidateTimer(id);
}
void EventLoop::doRunInLoopFuncs()
{
    callingFuncs_ = true;
    std::vector<Func> tmpFuncs;
    {
        std::lock_guard<std::mutex> lock(funcsMutex_);
        tmpFuncs.swap(funcs_);
    }
    for (auto funcs : tmpFuncs)
    {
        funcs();
    }
    callingFuncs_ = false;
}
void EventLoop::wakeup()
{
    if (!looping_)
        return;
    uint64_t tmp = 1;
#ifdef __linux__
    int ret = write(wakeupFd_, &tmp, sizeof(tmp));
#else
    int ret = write(wakeupFd_[1], &tmp, sizeof(tmp));
#endif
    if (ret < 0)
    {
        LOG_SYSERR << "wakeup error";
    }
}
void EventLoop::wakeupRead()
{
    uint64_t tmp;
#ifdef __linux__
    ssize_t ret = read(wakeupFd_, &tmp, sizeof(tmp));
#else
    ssize_t ret = read(wakeupFd_[0], &tmp, sizeof(tmp));
#endif
    if (ret < 0)
        LOG_SYSERR << "wakeup read error";
}
} // namespace trantor
