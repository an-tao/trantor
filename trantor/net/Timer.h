#pragma once

#include <trantor/utils/Date.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/callbacks.h>
#include <functional>
#include <atomic>
#include <iostream>
namespace trantor
{

    class Timer:public NonCopyable
    {
    public:
        Timer(const TimerCallback &cb,const Date &when,double interval):
                callback_(cb),
                when_(when),
                interval_(interval),
                repeat_(interval>0.0),
                timerSeq_(timersCreated_++)
        {

        }
        ~Timer(){
         //   std::cout<<"Timer unconstract!"<<std::endl;
        }
        void run() const;
        void restart(const Date& now);
        bool operator < (const Timer &t) const;
        bool operator > (const Timer &t) const;
        const Date& when() const {return when_;}
        bool isRepeat(){return repeat_;}
    private:
        TimerCallback callback_;
        Date when_;
        const bool repeat_;
        const double interval_;
        const int64_t timerSeq_;
        static std::atomic<int64_t> timersCreated_;
    };
};