// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

// taken from muduo and modified

#include "PollPoller.h"

#include "../Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <iostream>

using namespace trantor;

PollPoller::PollPoller(EventLoop *loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

void PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // XXX pollfds_ shouldn't change
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    int savedErrno = errno;
    if (numEvents > 0)
    {
        //LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
    {
        //LOG_TRACE << " nothing happened";
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            //LOG_SYSERR << "PollPoller::poll()";
        }
    }
    return;
}

void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList *activeChannels) const
{
    for (PollFdList::const_iterator pfd = pollfds_.begin();
         pfd != pollfds_.end() && numEvents > 0; ++pfd)
    {
        if (pfd->revents > 0)
        {
            --numEvents;
            auto ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->setRevents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

void PollPoller::updateChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    //LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    //std::cout<<"update chl sock="<<channel->fd()<<" index="<<channel->index()<<std::endl;
    if (channel->index() < 0)
    {
        // a new one, add to pollfds_
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->setIndex(idx);
        channels_[pfd.fd] = channel;
    }
    else
    {
        // update existing one
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent())
        {
            // ignore this pollfd
            pfd.fd = -channel->fd() - 1;
        }
    }
}

void PollPoller::removeChannel(Channel *channel)
{
    //begin
    Poller::assertInLoopThread();
    //std::cout<<"remove chl,sock="<<channel->fd()<<" index="<<channel->index()<<std::endl;
    //LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd &pfd = pollfds_[idx];
    (void)pfd;
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);
    (void)n;
    if (static_cast<size_t>(idx) == pollfds_.size() - 1)
    {
        pollfds_.pop_back();
    }
    else
    {
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->setIndex(idx);
        pollfds_.pop_back();
    }
}
