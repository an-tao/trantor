#include "Date.h"
#include <sys/time.h>
#include <cstdlib>

namespace trantor
{
    const Date Date::date()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t seconds = tv.tv_sec;
        return Date(seconds * MICRO_SECONDS_PRE_SEC + tv.tv_usec);
    }
    const Date Date::after(double second) const
    {
        return Date(microSecondsSinceEpoch_ + second * MICRO_SECONDS_PRE_SEC);
    }
    const Date Date::roundSecond() const
    {
        return Date(microSecondsSinceEpoch_ - (microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC));
    }
    const Date Date::roundDay() const
    {
        struct tm *t;
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
        t = localtime(&seconds);
        t->tm_hour=0;
        t->tm_min=0;
        t->tm_sec=0;
        return Date(mktime(t)*MICRO_SECONDS_PRE_SEC);
    }
    std::string Date::toFormattedString(bool showMicroseconds) const
    {
        char buf[32] = {0};
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
        struct tm tm_time;
        gmtime_r(&seconds, &tm_time);

        if (showMicroseconds)
        {
            int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                     microseconds);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }
        return buf;
    }
    std::string Date::toCustomedFormattedString(const std::string &fmtStr) const
    {
        char buf[256] = {0};
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
        struct tm tm_time;
        gmtime_r(&seconds, &tm_time);
        strftime (buf,sizeof(buf),fmtStr.c_str(),&tm_time);
        return std::string(buf);
    }
};