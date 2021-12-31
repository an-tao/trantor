/**
 *
 *  IoUringPoller.h
 *  Martin Chang
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2021, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

// TODO: Make this file comilable on Windows

#pragma once

#include "../Poller.h"
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/EventLoop.h>

#include <memory>
#include <map>

#ifdef __linux__
#include <liburing.h>
#endif

using EventList = std::vector<struct epoll_event>;

namespace trantor
{
class Channel;

struct IoUringData
{
    enum class Type
    {
        Epoll = 0,
        FileRead = 1,
        FileWrite = 2,
    };

    Type type;
    struct FileReadData
    {
        int fd;
    };
    union
    {
        FileReadData fileRead;
    };
};

class IoUringPoller : public Poller
{
  public:
    explicit IoUringPoller(EventLoop *loop);
    virtual ~IoUringPoller();
    virtual void poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;

  private:
    static const int kInitEventListSize = 16;
#ifdef __linux__
    int epollfd_;
    EventList events_;
    io_uring ring_;
    int iouringEventFd_;

    void update(int operation, Channel *channel);
    std::map<int, epoll_event> epoll_events_;
#ifndef NDEBUG
    using ChannelMap = std::map<int, Channel *>;
    ChannelMap channels_;
#endif
#endif
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void enqueueEpollPoll();
};
}  // namespace trantor
