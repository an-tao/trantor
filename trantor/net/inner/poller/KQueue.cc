#include "KQueue.h"
#include "Channel.h"
#include <trantor/utils/Logger.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>

namespace trantor
{

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
} // namespace

KQueue::KQueue(EventLoop *loop)
    : Poller(loop),
      _kqfd(kqueue()),
      _events(kInitEventListSize)
{
    assert(_kqfd >= 0);
}

KQueue::~KQueue()
{
    close(_kqfd);
}

void KQueue::resetAfterFork()
{
    close(_kqfd);
    _kqfd = kqueue();
    for (auto &ch : _channels)
    {
        ch.second.first = 0;
        if (ch.second.second->isReading() || ch.second.second->isWriting())
        {
            update(ch.second.second);
        }
    }
}

void KQueue::poll(int timeoutMs, ChannelList *activeChannels)
{
    struct timespec timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_nsec = (timeoutMs % 1000) * 1000000;

    int numEvents = kevent(_kqfd, NULL, 0, _events.data(), static_cast<int>(_events.size()), &timeout);
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
            LOG_SYSERR << "KQueue::poll()";
        }
    }
    return;
}

void KQueue::fillActiveChannels(int numEvents,
                                ChannelList *activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= _events.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(_events[i].udata);
        assert(_channels.find(channel->fd()) != _channels.end());
        int events = _events[i].filter;
        if (events == EVFILT_READ)
        {
            channel->setRevents(POLLIN);
        }
        else if (events == EVFILT_WRITE)
        {
            channel->setRevents(POLLOUT);
        }
        else
        {
            LOG_ERROR << "events=" << events;
            continue;
        }
        activeChannels->push_back(channel);
    }
}

void KQueue::updateChannel(Channel *channel)
{
    assertInLoopThread();
    const int index = channel->index();
    // LOG_TRACE << "fd = " << channel->fd()
    //           << " events = " << channel->events() << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            assert(_channels.find(channel->fd()) == _channels.end());
        }
        else
        { // index == kDeleted
            assert(_channels.find(channel->fd()) != _channels.end());
            assert(_channels[channel->fd()].second == channel);
        }
        update(channel);
        channel->setIndex(kAdded);
    }
    else
    {
        // update existing one
        int fd = channel->fd();
        (void)fd;
        assert(_channels.find(fd) != _channels.end());
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(channel);
            channel->setIndex(kDeleted);
        }
        else
        {
            update(channel);
        }
    }
}
void KQueue::removeChannel(Channel *channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    assert(_channels.find(fd) != _channels.end());
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);

    if (index == kAdded)
    {
        update(channel);
    }

    size_t n = _channels.erase(fd);
    (void)n;
    assert(n == 1);
    channel->setIndex(kNew);
}

void KQueue::update(Channel *channel)
{
    struct kevent ev[2];
    int n = 0;
    auto events = channel->events();
    int oldEvents = 0;
    if (_channels.find(channel->fd()) != _channels.end())
    {
        oldEvents = _channels[channel->fd()].first;
    }

    auto fd = channel->fd();
    _channels[fd] = {events, channel};

    if ((events & Channel::kReadEvent) && (!(oldEvents & Channel::kReadEvent)))
    {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)channel);
    }
    else if ((!(events & Channel::kReadEvent)) && (oldEvents & Channel::kReadEvent))
    {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)channel);
    }
    if ((events & Channel::kWriteEvent) && (!(oldEvents & Channel::kWriteEvent)))
    {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)channel);
    }
    else if ((!(events & Channel::kWriteEvent)) && (oldEvents & Channel::kWriteEvent))
    {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)channel);
    }
    kevent(_kqfd, ev, n, NULL, 0, NULL);
}

} // namespace trantor
