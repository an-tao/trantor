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
