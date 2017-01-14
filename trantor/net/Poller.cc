#include "Poller.h"
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <iostream>

namespace trantor
{
	static_assert(EPOLLIN == POLLIN,"EPOLLIN != POLLIN");
	static_assert(EPOLLPRI == POLLPRI,"EPOLLPRI != POLLPRI");
	static_assert(EPOLLOUT == POLLOUT,"EPOLLOUT != POLLOUT");
	static_assert(EPOLLRDHUP == POLLRDHUP,"EPOLLRDHUP != POLLRDHUP");
	static_assert(EPOLLERR == POLLERR,"EPOLLERR != POLLERR");
	static_assert(EPOLLHUP == POLLHUP,"EPOLLHUP != POLLHUP");

    namespace
    {
        const int kNew = -1;
        const int kAdded = 1;
        const int kDeleted = 2;
    }


    Poller::Poller(EventLoop *loop)
        : ownerLoop_(loop),
          epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
          events_(kInitEventListSize)
    {}
    Poller::~Poller()
    {
        close(epollfd_);

    }
    void Poller::poll(int timeoutMs, ChannelList* activeChannels)
    {
        int numEvents = ::epoll_wait(epollfd_,
                                     &*events_.begin(),
                                     static_cast<int>(events_.size()),
                                     timeoutMs);
        int savedErrno = errno;
        // Timestamp now(Timestamp::now());
        if (numEvents > 0) {
            std::cout << numEvents << " events happended" << std::endl;
            fillActiveChannels(numEvents, activeChannels);
            if (static_cast<size_t>(numEvents) == events_.size()) {
                events_.resize(events_.size() * 2);
            }
        } else if (numEvents == 0) {
            std::cout << "nothing happended" << std::endl;
        } else {
            // error happens, log uncommon ones
            if (savedErrno != EINTR) {
                errno = savedErrno;
                std::cout << "EPollPoller::poll()" << std::endl;
            }
        }
        return;

    }
    void Poller::fillActiveChannels(int numEvents,
                                       ChannelList* activeChannels) const
    {
        assert(static_cast<size_t>(numEvents) <= events_.size());
        for (int i = 0; i < numEvents; ++i) {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
            int fd = channel->fd();
            ChannelMap::const_iterator it = channels_.find(fd);
            assert(it != channels_.end());
            assert(it->second == channel);
#endif
            channel->setRevents(events_[i].events);
            activeChannels->push_back(channel);
        }
    }
    void Poller::updateChannel(Channel* channel)
    {
        assertInLoopThread();
        const int index = channel->index();
        //LOG_TRACE << "fd = " << channel->fd()
        //  << " events = " << channel->events() << " index = " << index;
        if (index == kNew || index == kDeleted) {
            // a new one, add with EPOLL_CTL_ADD
            int fd = channel->fd();
            if (index == kNew) {
                assert(channels_.find(fd) == channels_.end());
                channels_[fd] = channel;
            } else { // index == kDeleted
                assert(channels_.find(fd) != channels_.end());
                assert(channels_[fd] == channel);
            }

            channel->setIndex(kAdded);
            update(EPOLL_CTL_ADD, channel);
        } else {
            // update existing one with EPOLL_CTL_MOD/DEL
            int fd = channel->fd();
            (void)fd;
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
            assert(index == kAdded);
            if (channel->isNoneEvent()) {
                update(EPOLL_CTL_DEL, channel);
                channel->setIndex(kDeleted);
            } else {
                update(EPOLL_CTL_MOD, channel);
            }
        }
    }
    void Poller::removeChannel(Channel* channel)
    {
        Poller::assertInLoopThread();
        int fd = channel->fd();
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->isNoneEvent());
        int index = channel->index();
        assert(index == kAdded || index == kDeleted);
        size_t n = channels_.erase(fd);
        (void)n;
        assert(n == 1);

        if (index == kAdded)
        {
            update(EPOLL_CTL_DEL, channel);
        }
        channel->setIndex(kNew);
    }
    void Poller::update(int operation, Channel* channel)
    {
        struct epoll_event event;
        bzero(&event, sizeof event);
        event.events = channel->events();
        event.data.ptr = channel;
        int fd = channel->fd();
        if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
            if (operation == EPOLL_CTL_DEL) {
                // LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
            } else {
                //  LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
            }
        }
    }

}
