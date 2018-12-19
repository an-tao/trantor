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

#include <memory>
#include <unordered_map>
#include <vector>

typedef std::vector<struct kevent> EventList;

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
    static const int kInitEventListSize = 16;
    int _kqfd;
    EventList _events;
    typedef std::unordered_map<int, std::pair<int,Channel*>> ChannelMap;
    ChannelMap _channels;

    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(Channel *channel);
};
} // namespace trantor