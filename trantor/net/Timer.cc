#include <trantor/net/Timer.h>
#include <trantor/utils/Logger.h>
namespace trantor
{
    std::atomic<int64_t> Timer::timersCreated_;
    Timer::Timer(const TimerCallback &cb,const Date &when,double interval):
            callback_(cb),
            when_(when),
            interval_(interval),
            repeat_(interval>0.0),
            timerSeq_(timersCreated_++)
    {

    }
    Timer::Timer(TimerCallback &&cb,const Date &when,double interval):
            callback_(std::move(cb)),
            when_(when),
            interval_(interval),
            repeat_(interval>0.0),
            timerSeq_(timersCreated_++)
    {
        LOG_TRACE<<"Timer move contrustor";
    }
    void Timer::run() const{
        callback_();
    }
    void Timer::restart(const Date& now) {
        if(repeat_)
        {
            when_=now.after(interval_);
        }
        else
            when_=Date();
    }
    bool Timer::operator<(const Timer &t) const {
        return when_<t.when_;
    }
    bool Timer::operator>(const Timer &t) const {
        return when_>t.when_;
    }
};