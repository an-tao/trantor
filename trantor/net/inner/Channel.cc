/**
 *
 *  Channel.cc
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

#include "Channel.h"
#include <trantor/net/EventLoop.h>
#include <poll.h>
#include <iostream>
namespace trantor
{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    : _loop(loop), _fd(fd), _events(0), _revents(0), _index(-1), _tied(false)
{
}

void Channel::remove()
{
    assert(_events == kNoneEvent);
    _addedToLoop = false;
    _loop->removeChannel(this);
}

void Channel::update()
{
    _loop->updateChannel(this);
}

void Channel::handleEvent()
{
    // LOG_TRACE<<"_revents="<<_revents;
    if (_tied)
    {
        std::shared_ptr<void> guard = _tie.lock();
        if (guard)
        {
            handleEventSafely();
        }
    }
    else
    {
        handleEventSafely();
    }
}
void Channel::handleEventSafely()
{
    if (_eventCallback)
    {
        _eventCallback();
        return;
    }
    if ((_revents & POLLHUP) && !(_revents & POLLIN))
    {
        // LOG_TRACE<<"handle close";
        if (_closeCallback)
            _closeCallback();
    }
    if (_revents & (POLLNVAL | POLLERR))
    {
        // LOG_TRACE<<"handle error";
        if (_errorCallback)
            _errorCallback();
    }
#ifdef __linux__
    if (_revents & (POLLIN | POLLPRI | POLLRDHUP))
#else
    if (_revents & (POLLIN | POLLPRI))
#endif
    {
        // LOG_TRACE<<"handle read";
        if (_readCallback)
            _readCallback();
    }
    if (_revents & POLLOUT)
    {
        // LOG_TRACE<<"handle write";
        if (_writeCallback)
            _writeCallback();
    }
}

}  // namespace trantor
