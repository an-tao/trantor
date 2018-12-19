#include "Poller.h"
#ifdef __linux__
#include "poller/EpollPoller.h"
#else
#include "poller/KQueue.h"
#endif
using namespace trantor;
Poller *Poller::newPoller(EventLoop *loop)
{
#ifdef __linux__
    return new EpollPoller(loop);
#else
    return new KQueue(loop);
#endif
}
