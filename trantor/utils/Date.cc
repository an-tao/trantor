/**
 *
 *  Date.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#include "Date.h"
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <cstdlib>
#include <iostream>
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <time.h>
#endif

namespace trantor
{
#ifdef _WIN32
int gettimeofday(timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif
const Date Date::date()
{
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Date(seconds * MICRO_SECONDS_PRE_SEC + tv.tv_usec);
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Date(seconds * MICRO_SECONDS_PRE_SEC + tv.tv_usec);
#endif
}
const Date Date::after(double second) const
{
    return Date(microSecondsSinceEpoch_ + second * MICRO_SECONDS_PRE_SEC);
}
const Date Date::roundSecond() const
{
    return Date(microSecondsSinceEpoch_ -
                (microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC));
}
const Date Date::roundDay() const
{
    struct tm t;
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
#ifndef _WIN32
    localtime_r(&seconds, &t);
#else
    localtime_s(&t, &seconds);
#endif
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return Date(mktime(&t) * MICRO_SECONDS_PRE_SEC);
}
struct tm Date::tmStruct() const
{
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif
    return tm_time;
}
std::string Date::toFormattedString(bool showMicroseconds) const
{
    //  std::cout<<"toFormattedString"<<std::endl;
    char buf[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif

    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
        snprintf(buf,
                 sizeof(buf),
                 "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf,
                 sizeof(buf),
                 "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec);
    }
    return buf;
}
std::string Date::toCustomedFormattedString(const std::string &fmtStr,
                                            bool showMicroseconds) const
{
    char buf[256] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[10] = {0};
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
    sprintf(decimals, ".%06d", microseconds);
    return std::string(buf) + decimals;
}
void Date::toCustomedFormattedString(const std::string &fmtStr,
                                     char *str,
                                     size_t len) const
{
    // not safe
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif
    strftime(str, len, fmtStr.c_str(), &tm_time);
}
std::string Date::toFormattedStringLocal(bool showMicroseconds) const
{
    //  std::cout<<"toFormattedString"<<std::endl;
    char buf[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif

    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
        snprintf(buf,
                 sizeof(buf),
                 "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf,
                 sizeof(buf),
                 "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec);
    }
    return buf;
}
std::string Date::toDbStringLocal() const
{
    char buf[128] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif
    bool showMicroseconds =
        (microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC != 0);
    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
        snprintf(buf,
                 sizeof(buf),
                 "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        if (*this == roundDay())
        {
            snprintf(buf,
                     sizeof(buf),
                     "%4d-%02d-%02d",
                     tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1,
                     tm_time.tm_mday);
        }
        else
        {
            snprintf(buf,
                     sizeof(buf),
                     "%4d-%02d-%02d %02d:%02d:%02d",
                     tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1,
                     tm_time.tm_mday,
                     tm_time.tm_hour,
                     tm_time.tm_min,
                     tm_time.tm_sec);
        }
    }
    return buf;
}
std::string Date::toCustomedFormattedStringLocal(const std::string &fmtStr,
                                                 bool showMicroseconds) const
{
    char buf[256] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[10] = {0};
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PRE_SEC);
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

}  // namespace trantor
