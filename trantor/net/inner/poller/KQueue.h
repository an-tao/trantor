/**
 *
 *  KQueue.h
 *  An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/trantor
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *  Trantor
 *
 */

#pragma once
#include "../Poller.h"
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/EventLoop.h>

#if (defined(__unix__) && !defined(__linux__)) || \
    (defined(__APPLE__) && defined(__MACH__))
#define USE_KQUEUE
#include <memory>
#include <unordered_map>
#include <vector>
typedef std::vector<struct kevent> EventList;
#endif
namespace trantor
{
class Channel;

class KQueue : public Poller
{
  public:
    explicit KQueue(EventLoop *loop);
    virtual ~KQueue();
    virtual void poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;
    virtual void resetAfterFork() override;

  private:
#ifdef USE_KQUEUE
    static const int kInitEventListSize = 16;
    int _kqfd;
    EventList _events;
    typedef std::unordered_map<int, std::pair<int, Channel *>> ChannelMap;
    ChannelMap _channels;

    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(Channel *channel);
#endif
};

}  // namespace trantor