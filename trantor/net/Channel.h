/**
 *
 *  @file Channel.h
 *  @author An Tao
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
    using EventCallback = std::function<void()>;
    Channel(EventLoop *loop, int fd);
    void handleEvent();
    void setReadCallback(const EventCallback &cb)
    {
        readCallback_ = cb;
    };
    void setWriteCallback(const EventCallback &cb)
    {
        writeCallback_ = cb;
    };
    void setCloseCallback(const EventCallback &cb)
    {
        closeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback &cb)
    {
        errorCallback_ = cb;
    }
    void setEventCallback(const EventCallback &cb)
    {
        eventCallback_ = cb;
    }

    void setReadCallback(EventCallback &&cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback &&cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback &&cb)
    {
        closeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback &&cb)
    {
        errorCallback_ = std::move(cb);
    }
    void setEventCallback(EventCallback &&cb)
    {
        eventCallback_ = std::move(cb);
    }

    int fd() const
    {
        return fd_;
    }
    int events() const
    {
        return events_;
    }
    int revents() const
    {
        return revents_;
    }
    int setRevents(int revt)
    {
        // LOG_TRACE<<"revents="<<revt;
        revents_ = revt;
        return revt;
    };
    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    };
    int index()
    {
        return index_;
    };
    void setIndex(int index)
    {
        index_ = index;
    };
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }
    void remove();
    void tie(const std::shared_ptr<void> &obj)
    {
        tie_ = obj;
        tied_ = true;
    }
    EventLoop *ownerLoop()
    {
        return loop_;
    };

    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    bool isWriting() const
    {
        return events_ & kWriteEvent;
    }
    bool isReading() const
    {
        return events_ & kReadEvent;
    }

    void updateEvents(int events)
    {
        events_ = events;
        update();
    }
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

  private:
    void update();
    void handleEventSafely();
    EventLoop *loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;
    bool addedToLoop_{false};
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
    EventCallback eventCallback_;
    std::weak_ptr<void> tie_;
    bool tied_;
};
}  // namespace trantor
