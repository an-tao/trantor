/**
 *
 *  Poller.cc
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

#include "Poller.h"
#include <trantor/utils/Logger.h>
#ifdef __linux__
#include "poller/IoUringPoller.h"
#include "poller/EpollPoller.h"
#include <sys/utsname.h>
#elif defined _WIN32
#include "Wepoll.h"
#include "poller/EpollPoller.h"
#elif defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
#include "poller/KQueue.h"
#else
#include "poller/PollPoller.h"
#endif
using namespace trantor;
Poller *Poller::newPoller(EventLoop *loop)
{
#if defined __linux__
#if USE_LIBURING
    // Our io_uring poller requires kerenel >= 5.6 for polling file descriptors
    utsname buffer;
    uname(&buffer);
    std::string release(buffer.release);
    auto dot1 = release.find('.');
    if (dot1 == std::string::npos)
    {
        LOG_FATAL << "Unexpected version string format";
        abort();
    }
    auto dot2 = release.substr(dot1).find('.');
    if (dot2 == std::string::npos)
    {
        LOG_FATAL << "Unexpected version string format";
        abort();
    }
    int majorVersion = stoi(release.substr(0, dot1));
    int minorVersion = stoi(release.substr(dot1 + 1, dot2 - dot1));
    bool supportIoUring =
        majorVersion > 5 || (majorVersion == 5 && minorVersion >= 6);
    if (supportIoUring)
        return new IoUringPoller(loop);
#endif
    return new EpollPoller(loop);
#elif defined _WIN32
    return new EpollPoller(loop);
#elif defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
    return new KQueue(loop);
#else
    return new PollPoller(loop);
#endif
}
