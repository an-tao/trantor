#include "Poller.h"
#ifdef __linux__
#include "poller/EpollPoller.h"
#else
#include "poller/PollPoller.h"
#endif
using namespace trantor;
Poller *Poller::newPoller(EventLoop *loop)
{
#ifdef __linux__
    return new EpollPoller(loop);
#else
    return new PollPoller(loop);
#endif
}