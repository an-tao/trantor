// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#pragma once

#include "../Poller.h"

#include <vector>

struct pollfd;

namespace trantor
{

///
/// IO Multiplexing with poll(2).
///
    class PollPoller : public Poller
    {
    public:

        explicit PollPoller(EventLoop* loop);
        virtual ~PollPoller();
        virtual void poll(int timeoutMs, ChannelList* activeChannels) override ;
        virtual void updateChannel(Channel* channel) override ;
        virtual void removeChannel(Channel* channel) override ;

    private:
        void fillActiveChannels(int numEvents,
                                ChannelList* activeChannels) const;

        typedef std::vector<struct pollfd> PollFdList;
        PollFdList pollfds_;
        typedef std::map<int, Channel*> ChannelMap;
        ChannelMap channels_;
    };


}