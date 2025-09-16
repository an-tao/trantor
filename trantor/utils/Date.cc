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
#include "Funcs.h"
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <cstdlib>
#include <iostream>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
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
    tp->tv_sec = static_cast<long>(clock);
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif
const Date Date::date()
{
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    return Date(seconds * MICRO_SECONDS_PER_SEC + tv.tv_usec);
#else
    timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    return Date(seconds * MICRO_SECONDS_PER_SEC + tv.tv_usec);
#endif
}
const Date Date::after(double second) const
{
    return Date(microSecondsSinceEpoch_ +
                static_cast<int64_t>(
                    second * static_cast<double>(MICRO_SECONDS_PER_SEC)));
}
const Date Date::roundSecond() const
{
    return Date(microSecondsSinceEpoch_ -
                (microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC));
}
const Date Date::roundDay() const
{
    struct tm t;
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
#ifndef _WIN32
    localtime_r(&seconds, &t);
#else
    localtime_s(&t, &seconds);
#endif
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return Date(static_cast<int64_t>(mktime(&t)) * MICRO_SECONDS_PER_SEC);
}
struct tm Date::tmStruct() const
{
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
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
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif

    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC);
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
std::string Date::toCustomFormattedString(const std::string &fmtStr,
                                          bool showMicroseconds) const
{
    char buf[256] = {0};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
    struct tm tm_time;
#ifndef _WIN32
    gmtime_r(&seconds, &tm_time);
#else
    gmtime_s(&tm_time, &seconds);
#endif
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[12] = {0};
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC);
    snprintf(decimals, sizeof(decimals), ".%06d", microseconds);
    return std::string(buf) + decimals;
}
void Date::toCustomFormattedString(const std::string &fmtStr,
                                   char *str,
                                   size_t len) const
{
    // not safe
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
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
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif

    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC);
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
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif
    bool showMicroseconds =
        (microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC != 0);
    if (showMicroseconds)
    {
        int microseconds =
            static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC);
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
std::string Date::toDbString() const
{
    return after(static_cast<double>(-timezoneOffset())).toDbStringLocal();
}

Date Date::fromDbStringLocal(const std::string &datetime)
{
    unsigned int year = 0U, month = 0U, day = 0U, hour = 0U, minute = 0U,
                 second = 0U, microSecond = 0U;
    std::vector<std::string> &&v = splitString(datetime, " ");

    if (v.size() == 0)
    {
        throw std::invalid_argument("Invalid date string: " + datetime);
    }
    const std::vector<std::string> date = splitString(v[0], "-");
    if (date.size() != 3)
    {
        throw std::invalid_argument("Invalid date string: " + datetime);
    }
    if (v.size() == 1)
    {
        // Fromat YYYY-MM-DD is given
        try
        {
            year = std::stol(date[0]);
            month = std::stol(date[1]);
            day = std::stol(date[2]);
        }
        catch (...)
        {
            throw std::invalid_argument("Invalid date string: " + datetime);
        }
        return Date(year, month, day, hour, minute, second, microSecond);
    }

    if (v.size() == 2)
    {
        // Format YYYY-MM-DD HH:MM:SS[.UUUUUU] is given
        try
        {
            year = static_cast<unsigned int>(std::stoul(date[0]));
            month = static_cast<unsigned int>(std::stoul(date[1]));
            day = static_cast<unsigned int>(std::stoul(date[2]));
            std::vector<std::string> time = splitString(v[1], ":");
            if (2 < time.size())
            {
                hour = static_cast<unsigned int>(std::stoul(time[0]));
                minute = static_cast<unsigned int>(std::stoul(time[1]));
                auto seconds = splitString(time[2], ".");
                second = static_cast<unsigned int>(std::stoul(seconds[0]));
                if (1 < seconds.size())
                {
                    if (seconds[1].length() > 6)
                    {
                        seconds[1].resize(6);
                    }
                    else if (seconds[1].length() < 6)
                    {
                        seconds[1].append(6 - seconds[1].length(), '0');
                    }
                    microSecond =
                        static_cast<unsigned int>(std::stoul(seconds[1]));
                }
            }
        }
        catch (...)
        {
            throw std::invalid_argument("Invalid date string: " + datetime);
        }
        return Date(year, month, day, hour, minute, second, microSecond);
    }

    throw std::invalid_argument("Invalid date string: " + datetime);
}

Date Date::fromDbString(const std::string &datetime)
{
    return fromDbStringLocal(datetime).after(
        static_cast<double>(timezoneOffset()));
}

std::string Date::toCustomFormattedStringLocal(const std::string &fmtStr,
                                               bool showMicroseconds) const
{
    char buf[256]{};
    time_t seconds =
        static_cast<time_t>(microSecondsSinceEpoch_ / MICRO_SECONDS_PER_SEC);
    struct tm tm_time;
#ifndef _WIN32
    localtime_r(&seconds, &tm_time);
#else
    localtime_s(&tm_time, &seconds);
#endif
    strftime(buf, sizeof(buf), fmtStr.c_str(), &tm_time);
    if (!showMicroseconds)
        return std::string(buf);
    char decimals[12]{};
    int microseconds =
        static_cast<int>(microSecondsSinceEpoch_ % MICRO_SECONDS_PER_SEC);
    snprintf(decimals, sizeof(decimals), ".%06d", microseconds);
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
    tm.tm_isdst = -1;
    time_t epoch;
    tm.tm_year = static_cast<int>(year - 1900);
    tm.tm_mon = static_cast<int>(month - 1);
    tm.tm_mday = static_cast<int>(day);
    tm.tm_hour = static_cast<int>(hour);
    tm.tm_min = static_cast<int>(minute);
    tm.tm_sec = static_cast<int>(second);
    epoch = mktime(&tm);
    microSecondsSinceEpoch_ =
        static_cast<int64_t>(epoch) * MICRO_SECONDS_PER_SEC + microSecond;
}

}  // namespace trantor
