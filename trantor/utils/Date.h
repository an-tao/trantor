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
    bool operator==(const Date &date) const
    {
        return microSecondsSinceEpoch_ == date.microSecondsSinceEpoch_;
    }
    bool operator!=(const Date &date) const
    {
        return microSecondsSinceEpoch_ != date.microSecondsSinceEpoch_;
    }
    bool operator<(const Date &date) const
    {
        return microSecondsSinceEpoch_ < date.microSecondsSinceEpoch_;
    }
    bool operator>(const Date &date) const
    {
        return microSecondsSinceEpoch_ > date.microSecondsSinceEpoch_;
    }
    bool operator>=(const Date &date) const
    {
        return microSecondsSinceEpoch_ >= date.microSecondsSinceEpoch_;
    }
    bool operator<=(const Date &date) const
    {
        return microSecondsSinceEpoch_ <= date.microSecondsSinceEpoch_;
    }
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    struct tm tmStruct() const;
    std::string toFormattedString(bool showMicroseconds) const;                             //UTC
    std::string toCustomedFormattedString(const std::string &fmtStr) const;                 //UTC
    void toCustomedFormattedString(const std::string &fmtStr, char *str, size_t len) const; //UTC
    bool isSameSecond(const Date &date) const
    {
        return microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC == date.microSecondsSinceEpoch_ / MICRO_SECONDS_PRE_SEC;
    }
    const Date roundSecond() const;
    const Date roundDay() const;
    void swap(Date &that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

  private:
    explicit Date(int64_t microSec) : microSecondsSinceEpoch_(microSec){};
    int64_t microSecondsSinceEpoch_ = 0;
};
}; // namespace trantor