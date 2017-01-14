#include "Date.h"
#include <sys/time.h>
#include <cstdlib>
#define MICRO_SECONDS_PRE_SEC 1000000
namespace trantor
{
    const Date Date::date() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t seconds = tv.tv_sec;
        return Date(seconds * MICRO_SECONDS_PRE_SEC + tv.tv_usec);
    }
    Date Date::after(double second) const {
        return Date(microSecondsSinceEpoch_+second*MICRO_SECONDS_PRE_SEC);
    }
};