#include <trantor/net/EventLoop.h>
#include <trantor/net/Poller.h>
#include <trantor/net/TimerQueue.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/Channel.h>

#include <thread>
#include <assert.h>
#include <poll.h>
#include <iostream>
#include <sys/eventfd.h>
#include <functional>
#include <unistd.h>
#include <algorithm>
#include <signal.h>

using namespace std;
namespace trantor
{
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0) {
            std::cout << "Failed in eventfd" << std::endl;
            abort();
        }

        return evtfd;
    }

    __thread EventLoop * t_loopInThisThread = 0;
    const int kPollTimeMs = 10000;
    EventLoop::EventLoop()
            : looping_(false),
              threadId_(std::this_thread::get_id()),
              quit_(false),
              poller_(new Poller(this)),
              currentActiveChannel_(NULL),
              eventHandling_(false),
              timerQueue_(new TimerQueue(this)),
              wakeupFd_(createEventfd()),
              wakeupChannelPtr_(new Channel(this,wakeupFd_))
    {
        assert(t_loopInThisThread == 0);
        t_loopInThisThread = this;

        wakeupChannelPtr_->setReadCallback(
                std::bind(&EventLoop::wakeupRead, this));
        wakeupChannelPtr_->enableReading();

    }



    EventLoop::~EventLoop()
    {
        assert(!looping_);
        t_loopInThisThread = NULL;
    }
    EventLoop* EventLoop::getEventLoopOfCurrentThread()
    {
        return t_loopInThisThread;
    }
    void EventLoop::updateChannel(Channel* channel)
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
        // if (!isInLoopThread())
        // {
        //   wakeup();
        // }
    }
    void EventLoop::loop()
    {
        assert(!looping_);
        assertInLoopThread();
        looping_ = true;
        quit_ = false;

        while (!quit_) {
            activeChannels_.clear();
            poller_->poll(kPollTimeMs, &activeChannels_);

            // TODO sort channel by priority
            //std::cout<<"after ->poll()"<<std::endl;
            eventHandling_ = true;
            for (auto it = activeChannels_.begin();
                 it != activeChannels_.end(); ++it) {
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
        exit(-1);
    }
    void EventLoop::runInLoop(const Func& cb)
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
    void EventLoop::runInLoop(Func&& cb)
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
    void EventLoop::queueInLoop(const Func& cb)
    {
        {
            std::lock_guard<std::mutex> lock(funcsMutex_);
            funcs_.push_back(cb);
        }

        if (!isInLoopThread() || callingFuncs_ ||!looping_)
        {
            wakeup();
        }
    }
    void EventLoop::queueInLoop(Func&& cb)
    {
        {
            std::lock_guard<std::mutex> lock(funcsMutex_);
            funcs_.push_back(std::move(cb));
        }

        if (!isInLoopThread() || callingFuncs_ ||!looping_)
        {
            wakeup();
        }
    }

    void EventLoop::runAt(const Date& time, const Func& cb){
        timerQueue_->addTimer(cb,time,0);
    }
    void EventLoop::runAt(const Date& time, Func&& cb){
        timerQueue_->addTimer(std::move(cb),time,0);
    }
    void EventLoop::runAfter(double delay, const Func& cb){
        runAt(Date::date().after(delay),cb);
    }
    void EventLoop::runAfter(double delay, Func&& cb){
        runAt(Date::date().after(delay),std::move(cb));
    }
    void EventLoop::runEvery(double interval, const Func& cb){
        timerQueue_->addTimer(cb,Date::date(),interval);
    }
    void EventLoop::runEvery(double interval, Func&& cb){
        timerQueue_->addTimer(std::move(cb),Date::date(),interval);
    }
    void EventLoop::doRunInLoopFuncs()
    {
        callingFuncs_=true;
        std::vector<Func> tmpFuncs;
        {
            std::lock_guard<std::mutex> lock(funcsMutex_);
            tmpFuncs.swap(funcs_);
        }
        for(auto funcs:tmpFuncs)
        {
            funcs();
        }
        callingFuncs_ = false;
    }
    void EventLoop::wakeup()
    {
        uint64_t tmp=1;
        int ret=write(wakeupFd_,&tmp,sizeof(tmp));
        if(ret<0)
        {
            LOG_SYSERR<<"wakeup error";
        }
    }
    void EventLoop::wakeupRead()
    {
        uint64_t tmp;
        ssize_t ret=read(wakeupFd_,&tmp,sizeof(tmp));
        if(ret<0)
            LOG_SYSERR<<"wakeup read error";
    }
}
