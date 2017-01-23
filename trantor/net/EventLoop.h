#pragma once
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/Channel.h>
#include <trantor/net/EventLoop.h>
#include <trantor/utils/Date.h>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
namespace trantor
{
    class Poller;
    class TimerQueue;
    typedef std::vector<Channel*> ChannelList;
    typedef std::function<void ()> Func;
    class EventLoop:NonCopyable
    {
    public:
        EventLoop();
        ~EventLoop();
        void loop();
        void quit();
        void assertInLoopThread()
        {
            if(!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        };
        bool isInLoopThread() const { return threadId_ == std::this_thread::get_id();};
        static EventLoop* getEventLoopOfCurrentThread();
        void updateChannel(Channel *chl);
        void removeChannel(Channel *chl);
        void runInLoop(const Func &f);
        void queueInLoop(const Func &f);
        void wakeup();
        void wakeupRead();
        ///
        /// Runs callback at 'time'.
        /// Safe to call from other threads.
        ///
        void runAt(const Date& time, const Func& cb);
        ///
        /// Runs callback after @c delay seconds.
        /// Safe to call from other threads.
        ///
        void runAfter(double delay, const Func& cb);
        ///
        /// Runs callback every @c interval seconds.
        /// Safe to call from other threads.
        ///
        void runEvery(double interval, const Func& cb);
        //int getAioEventFd();
        //io_context_t getAioContext() {return ctx_;};
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
        bool callingFuncs_=false;
        int wakeupFd_;
        std::unique_ptr<Channel> wakeupChannelPtr_;

        void doRunInLoopFuncs();
    };
}
