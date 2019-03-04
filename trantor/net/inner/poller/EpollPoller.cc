/**
 *
 *  EpollPoller.cc
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

#include <trantor/utils/Logger.h>
#include "Channel.h"
#include "EpollPoller.h"
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <iostream>

namespace trantor
{
static_assert(EPOLLIN == POLLIN, "EPOLLIN != POLLIN");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI != POLLPRI");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT != POLLOUT");
static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP != POLLRDHUP");
static_assert(EPOLLERR == POLLERR, "EPOLLERR != POLLERR");
static_assert(EPOLLHUP == POLLHUP, "EPOLLHUP != POLLHUP");

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
} // namespace

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      _epollfd(::epoll_create1(EPOLL_CLOEXEC)),
      _events(kInitEventListSize)
{
}
EpollPoller::~EpollPoller()
{
    close(_epollfd);
}
void EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::epoll_wait(_epollfd,
                                 &*_events.begin(),
                                 static_cast<int>(_events.size()),
                                 timeoutMs);
    int savedErrno = errno;
    // Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == _events.size())
        {
            _events.resize(_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //std::cout << "nothing happended" << std::endl;
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollEpollPoller::poll()";
        }
    }
    return;
}
void EpollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= _events.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(_events[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = _channels.find(fd);
        assert(it != _channels.end());
        assert(it->second == channel);
#endif
        channel->setRevents(_events[i].events);
        activeChannels->push_back(channel);
    }
    //LOG_TRACE<<"active Channels num:"<<activeChannels->size();
}
void EpollPoller::updateChannel(Channel *channel)
{
    assertInLoopThread();
    const int index = channel->index();
    //LOG_TRACE << "fd = " << channel->fd()
    //  << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
#ifndef NDEBUG
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(_channels.find(fd) == _channels.end());
            _channels[fd] = channel;
        }
        else
        { // index == kDeleted
            assert(_channels.find(fd) != _channels.end());
            assert(_channels[fd] == channel);
        }
#endif
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
#ifndef NDEBUG
        int fd = channel->fd();
        (void)fd;
        assert(_channels.find(fd) != _channels.end());
        assert(_channels[fd] == channel);
#endif
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EpollPoller::removeChannel(Channel *channel)
{
    EpollPoller::assertInLoopThread();
#ifndef NDEBUG
    int fd = channel->fd();
    assert(_channels.find(fd) != _channels.end());
    assert(_channels[fd] == channel);
    size_t n = _channels.erase(fd);
    (void)n;
    assert(n == 1);
#endif
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}
void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(_epollfd, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            // LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
        else
        {
            //  LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}

} // namespace trantor
