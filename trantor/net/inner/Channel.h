/**
 *
 *  Channel.h
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

#pragma once

#include <trantor/utils/Logger.h>
#include <trantor/utils/NonCopyable.h>
#include <functional>
#include <assert.h>
#include <memory>
namespace trantor
{
class EventLoop;
class Channel : NonCopyable
{
  public:
    typedef std::function<void()> EventCallback;
    Channel(EventLoop *loop, int fd);
    void handleEvent();
    void setReadCallback(const EventCallback &cb)
    {
        _readCallback = cb;
    };
    void setWriteCallback(const EventCallback &cb)
    {
        _writeCallback = cb;
    };
    void setCloseCallback(const EventCallback &cb)
    {
        _closeCallback = cb;
    }
    void setErrorCallback(const EventCallback &cb)
    {
        _errorCallback = cb;
    }
    void setEventCallback(const EventCallback &cb)
    {
        _eventCallback = cb;
    }

    void setReadCallback(EventCallback &&cb)
    {
        _readCallback = std::move(cb);
    }
    void setWriteCallback(EventCallback &&cb)
    {
        _writeCallback = std::move(cb);
    }
    void setCloseCallback(EventCallback &&cb)
    {
        _closeCallback = std::move(cb);
    }
    void setErrorCallback(EventCallback &&cb)
    {
        _errorCallback = std::move(cb);
    }
    void setEventCallback(EventCallback &&cb)
    {
        _eventCallback = std::move(cb);
    }

    int fd() const
    {
        return _fd;
    }
    int events() const
    {
        return _events;
    }
    int revents() const
    {
        return _revents;
    }
    int setRevents(int revt)
    {
        // LOG_TRACE<<"revents="<<revt;
        _revents = revt;
        return revt;
    };
    bool isNoneEvent() const
    {
        return _events == kNoneEvent;
    };
    int index()
    {
        return _index;
    };
    void setIndex(int index)
    {
        _index = index;
    };
    void disableAll()
    {
        _events = kNoneEvent;
        update();
    }
    void remove();
    void tie(const std::shared_ptr<void> &obj)
    {
        _tie = obj;
        _tied = true;
    }
    EventLoop *ownerLoop()
    {
        return _loop;
    };

    void enableReading()
    {
        _events |= kReadEvent;
        update();
    }
    void disableReading()
    {
        _events &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        _events |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        _events &= ~kWriteEvent;
        update();
    }

    bool isWriting() const
    {
        return _events & kWriteEvent;
    }
    bool isReading() const
    {
        return _events & kReadEvent;
    }

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

  private:
    void update();
    void handleEventSafely();
    EventLoop *_loop;
    const int _fd;
    int _events;
    int _revents;
    int _index;
    bool _addedToLoop = false;
    EventCallback _readCallback;
    EventCallback _writeCallback;
    EventCallback _errorCallback;
    EventCallback _closeCallback;
    EventCallback _eventCallback;
    std::weak_ptr<void> _tie;
    bool _tied;
};
};  // namespace trantor
