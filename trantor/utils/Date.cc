#include "Date.h"
#include <sys/time.h>
#include <cstdlib>
#include <iostream>
#include <string.h>

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
    struct tm t;
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    localtime_r(&seconds,&t);
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return Date(mktime(&t) * MICRO_SECONDS_PRE_SEC);
}
struct tm Date::tmStruct() const
{
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    return tm_time;
}
std::string Date::toFormattedString(bool showMicroseconds) const
{
    //  std::cout<<"toFormattedString"<<std::endl;
    char buf[128] = {0};
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
std::string Date::toCustomedFormattedString(const std::string &fmtStr, bool showMicroseconds) const
{
    char buf[256] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[10] = {0};
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
    sprintf(decimals, ".%06d", microseconds);
    return std::string(buf) + decimals;
}
void Date::toCustomedFormattedString(const std::string &fmtStr, char *str, size_t len) const
{
    //not safe
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    strftime(str, len, fmtStr.c_str(), &tm_time);
}
std::string Date::toFormattedStringLocal(bool showMicroseconds) const
{
    //  std::cout<<"toFormattedString"<<std::endl;
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);

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
std::string Date::toDbStringLocal() const
{
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    bool showMicroseconds = (microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC != 0);
    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        if (*this == roundDay())
        {
            snprintf(buf, sizeof(buf), "%4d-%02d-%02d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }
    }
    return buf;
}
std::string Date::toCustomedFormattedStringLocal(const std::string &fmtStr, bool showMicroseconds) const
{
    char buf[256] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[10] = {0};
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
    sprintf(decimals, ".%06d", microseconds);
    return std::string(buf) + decimals;
}
Date::Date(unsigned int year,
           unsigned int month,
           unsigned int day,
           unsigned int hour,
           unsigned int minute,
           unsigned int second,
           unsigned int microSecond)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    time_t epoch;
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    epoch = mktime(&tm);
    microSecondsSinceEpoch_ = epoch * MICRO_SECONDS_PRE_SEC + microSecond;
}
}; // namespace trantor
