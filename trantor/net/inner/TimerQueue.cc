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
        std::cerr<<"create timerfd failed!"<<std::endl;
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(const Date &when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Date::date().microSecondsSinceEpoch();
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
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
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

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    const Date now=Date::date();
    readTimerfd(timerfd_, now);

    std::vector<TimerPtr> expired = getExpired(now);

    callingExpiredTimers_ = true;
    //cancelingTimers_.clear();
    // safe to callback outside critical section
    for (auto timerPtr:expired)
    {
        timerPtr->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}
#else
int howMuchTimeFromNow(const Date &when)
{
    int64_t microSeconds=when.microSecondsSinceEpoch()-Date::date().microSecondsSinceEpoch();
    if (microSeconds < 1000)
    {
        microSeconds = 1000;
    }
    return static_cast<int>(microSeconds / 1000);
}
void TimerQueue::processTimers()  {
    loop_->assertInLoopThread();
    const Date now=Date::date();

    std::vector<TimerPtr> expired = getExpired(now);

    callingExpiredTimers_ = true;
    //cancelingTimers_.clear();
    // safe to callback outside critical section
    for (auto timerPtr:expired)
    {
        timerPtr->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}
#endif
///////////////////////////////////////
TimerQueue::TimerQueue(EventLoop* loop)
        : loop_(loop),
#ifdef __linux__
          timerfd_(createTimerfd()),
          timerfdChannelPtr_(new Channel(loop, timerfd_)),
#endif
          timers_(),
          callingExpiredTimers_(false)
{
#ifdef __linux__
    timerfdChannelPtr_->setReadCallback(
            std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannelPtr_->enableReading();
#endif
}

TimerQueue::~TimerQueue()
{
#ifdef __linux__
    timerfdChannelPtr_->disableAll();
    timerfdChannelPtr_->remove();
    ::close(timerfd_);
#endif
}

void TimerQueue::addTimer(const TimerCallback &cb, const Date &when, double interval) {
    std::shared_ptr<Timer> timerPtr=std::make_shared<Timer>(cb,when,interval);
    //timers_.push(timerPtr);
    loop_->runInLoop([=](){
        addTimerInLoop(timerPtr);
    });
}
void TimerQueue::addTimer(TimerCallback &&cb, const Date &when, double interval) {
    std::shared_ptr<Timer> timerPtr=std::make_shared<Timer>(std::move(cb),when,interval);
    //timers_.push(timerPtr);
    loop_->runInLoop([=](){
        addTimerInLoop(timerPtr);
    });
}
void TimerQueue::addTimerInLoop(const TimerPtr &timer) {
    loop_->assertInLoopThread();
    if(insert(timer))
    {
        //the earliest timer changed
#ifdef __linux__
        resetTimerfd(timerfd_,timer->when());
#endif
    }
}

bool TimerQueue::insert(const TimerPtr &timerPtr)
{
    loop_->assertInLoopThread();
    bool earliestChanged = false;
    if (timers_.size()==0 || *timerPtr < *timers_.top())
    {
        earliestChanged = true;
    }
    timers_.push(timerPtr);
    //std::cout<<"after push new timer:"<<timerPtr->when().microSecondsSinceEpoch()/1000000<<std::endl;
    return earliestChanged;
}
#ifndef __linux__
int TimerQueue::getTimeout() const
{
    loop_->assertInLoopThread();
    if (timers_.empty())
    {
        return 10000;
    }
    else
    {
        return howMuchTimeFromNow(timers_.top()->when());
    }
}
#endif

std::vector<TimerPtr> TimerQueue::getExpired(const Date &now) {
    std::vector<TimerPtr> expired;
    while(!timers_.empty())
    {
        if(timers_.top()->when()<now)
        {
            expired.push_back(timers_.top());
            timers_.pop();
        }
        else
            break;
    }
    return expired;
}
void TimerQueue::reset(const std::vector<TimerPtr>& expired,const Date &now)
{
    for(auto timerPtr:expired)
    {
        if(timerPtr->isRepeat())
        {
            timerPtr->restart(now);
            insert(timerPtr);
        }
    }
#ifdef __linux__
    if (!timers_.empty())
    {
        const Date nextExpire = timers_.top()->when();
        resetTimerfd(timerfd_, nextExpire);
    }
#endif
}
