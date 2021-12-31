/**
 *
 *  IoUringPoller.cc
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

#include <trantor/utils/Logger.h>
#include "Channel.h"
#include "IoUringPoller.h"
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

#ifdef __linux__
namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace
IoUringPoller::IoUringPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (io_uring_queue_init(kInitEventListSize, &ring_, 0) < 0)
    {
        LOG_FATAL << "Failed to initialize io_uring: " << strerror(-errno);
        abort();
    }
    enqueueEpollPoll();
    io_uring_submit(&ring_);
}
IoUringPoller::~IoUringPoller()
{
    close(epollfd_);
    io_uring_queue_exit(&ring_);
}
void IoUringPoller::enqueueEpollPoll()
{
    auto sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_poll_add(sqe, epollfd_, POLLIN);
    auto data = new IoUringData;
    data->type = IoUringData::Type::Epoll;
    io_uring_sqe_set_data(sqe, data);
}
void IoUringPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    (void)timeoutMs;
    // Submit IO that might have been submitted since last time
    io_uring_submit(&ring_);
    bool epollHasEvent = false;
    while (true)
    {
        struct io_uring_cqe *cqe;
        int status = io_uring_peek_cqe(&ring_, &cqe);
        if (status < 0)
            LOG_SYSERR << "io_uring_peek_cqe";
        if (status == 0)
            break;

        auto data = (IoUringData *)io_uring_cqe_get_data(cqe);
        if (data->type == IoUringData::Type::Epoll)
        {
            epollHasEvent = true;
            enqueueEpollPoll();
        }

        io_uring_cqe_seen(&ring_, cqe);
        delete data;
    }
    if (!epollHasEvent)
    {
        io_uring_submit(&ring_);
        return;
    }

    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 0);
    int savedErrno = errno;
    // Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        // LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        // std::cout << "nothing happended" << std::endl;
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollIoUringPoller::poll()";
        }
    }
    io_uring_submit(&ring_);
}
void IoUringPoller::fillActiveChannels(int numEvents,
                                       ChannelList *activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
    // LOG_TRACE<<"active Channels num:"<<activeChannels->size();
}
void IoUringPoller::updateChannel(Channel *channel)
{
    assertInLoopThread();
    assert(channel->fd() >= 0);

    const int index = channel->index();
    // LOG_TRACE << "fd = " << channel->fd()
    //  << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
// a new one, add with EPOLL_CTL_ADD
#ifndef NDEBUG
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else
        {  // index == kDeleted
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
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
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
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
void IoUringPoller::removeChannel(Channel *channel)
{
    IoUringPoller::assertInLoopThread();
#ifndef NDEBUG
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    size_t n = channels_.erase(fd);
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
void IoUringPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            // LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) <<
            // " fd =" << fd;
        }
        else
        {
            //  LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation)
            //  << " fd =" << fd;
        }
    }
}
#else
IoUringPoller::IoUringPoller(EventLoop *loop) : Poller(loop)
{
    assert(false);
}
IoUringPoller::~IoUringPoller()
{
}
void IoUringPoller::poll(int, ChannelList *)
{
}
void IoUringPoller::updateChannel(Channel *)
{
}
void IoUringPoller::removeChannel(Channel *)
{
}

#endif
}  // namespace trantor
