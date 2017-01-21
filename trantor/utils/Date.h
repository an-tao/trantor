#pragma once

#include <stdint.h>
#include <string>

#define MICRO_SECONDS_PRE_SEC 1000000

namespace trantor
{
class Date
{
  public:
    Date() : microSecondsSinceEpoch_(0){};
    ~Date(){};
    const Date after(double second) const;
    static const Date date();
    bool operator<(const Date &date) const
    {
        return microSecondsSinceEpoch_ < date.microSecondsSinceEpoch_;
    }
    bool operator>(const Date &date) const
    {
        return microSecondsSinceEpoch_ > date.microSecondsSinceEpoch_;
    }
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    std::string toFormattedString(bool showMicroseconds) const; //UTC
    std::string toCustomedFormattedString(const std::string &fmtStr) const;//UTC
    bool isSameSecond(const Date &date) const
    {
        return microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC == date.microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC;
    }
    const Date roundSecond() const;
    const Date roundDay() const;

  private:
    Date(int64_t microSec) : microSecondsSinceEpoch_(microSec){};
    int64_t microSecondsSinceEpoch_ = 0;
};
};