#include <trantor/net/EventLoop.h>
#include <trantor/net/Poller.h>
#include <trantor/net/TimerQueue.h>
#include <thread>
#include <assert.h>
#include <poll.h>
#include <iostream>
#include <sys/eventfd.h>
#include <functional>
#include <unistd.h>
#include <algorithm>


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
              timerQueue_(new TimerQueue(this))
    {
        assert(t_loopInThisThread == 0);
        t_loopInThisThread = this;

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
            std::cout << "looping" << endl;
            //doPendingFunctors();
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

    void EventLoop::queueInLoop(const Func& cb)
    {
        {
            std::lock_guard<std::mutex> lock(funcsMutex_);
            funcs_.push(cb);
        }

        if (!isInLoopThread() || callingFuncs_)
        {
            wakeup();
        }
    }


    void EventLoop::runAt(const Date& time, const Func& cb){
        timerQueue_->addTimer(cb,time,0);
    }

    void EventLoop::runAfter(double delay, const Func& cb){
        runAt(Date::date().after(delay),cb);
    }

    void EventLoop::runEvery(double interval, const Func& cb){
        timerQueue_->addTimer(cb,Date::date(),interval);
    }
}
