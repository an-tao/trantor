#pragma once

#include <trantor/utils/Date.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/callbacks.h>
#include <functional>
#include <atomic>
#include <iostream>
namespace trantor
{
    typedef uint64_t TimerId;
    class Timer:public NonCopyable
    {
    public:
        Timer(const TimerCallback &cb,const Date &when,double interval,TimerId id);
        Timer(TimerCallback &&cb,const Date &when,double interval,TimerId id);
        ~Timer(){
         //   std::cout<<"Timer unconstract!"<<std::endl;
        }
        void run() const;
        void restart(const Date& now);
        bool operator < (const Timer &t) const;
        bool operator > (const Timer &t) const;
        const Date& when() const {return when_;}
        bool isRepeat(){return repeat_;}
        TimerId id(){return _id;}
    private:
        TimerCallback callback_;
        Date when_;
        const double interval_;
        const bool repeat_;
        const int64_t timerSeq_;
        TimerId _id;
        static std::atomic<int64_t> timersCreated_;
    };
};