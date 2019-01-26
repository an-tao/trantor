/**
 *
 *  Timer.cc
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

#include "Timer.h"
#include <trantor/utils/Logger.h>
namespace trantor
{
std::atomic<TimerId> Timer::_timersCreated = ATOMIC_VAR_INIT(0);
Timer::Timer(const TimerCallback &cb,
             const Date &when,
             double interval)
    : _callback(cb),
      _when(when),
      _interval(interval),
      _repeat(interval > 0.0),
      _id(_timersCreated++)

{
}
Timer::Timer(TimerCallback &&cb,
             const Date &when,
             double interval)
    : _callback(std::move(cb)),
      _when(when),
      _interval(interval),
      _repeat(interval > 0.0),
      _id(_timersCreated++)
{
    //LOG_TRACE<<"Timer move contrustor";
}
void Timer::run() const
{
    _callback();
}
void Timer::restart(const Date &now)
{
    if (_repeat)
    {
        _when = now.after(_interval);
    }
    else
        _when = Date();
}
bool Timer::operator<(const Timer &t) const
{
    return _when < t._when;
}
bool Timer::operator>(const Timer &t) const
{
    return _when > t._when;
}
}; // namespace trantor
