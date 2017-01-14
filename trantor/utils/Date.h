#pragma  once

#include <stdint.h>
namespace trantor
{
    class Date
    {
    public:
        Date():microSecondsSinceEpoch_(0){};
        ~Date(){};
        Date after(double second) const;
        static const Date date();
        bool operator < (const Date &date) const {
            return microSecondsSinceEpoch_ < date.microSecondsSinceEpoch_;
        }
        bool operator > (const Date &date) const {
            return microSecondsSinceEpoch_ > date.microSecondsSinceEpoch_;
        }
        int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
    private:
        Date(int64_t microSec):microSecondsSinceEpoch_(microSec){};
        int64_t microSecondsSinceEpoch_=0;
    };
};