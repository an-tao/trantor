/**
 *
 *  EventLoopThreadPool.h
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

#pragma once

#include <trantor/net/EventLoopThread.h>
#include <vector>
namespace trantor
{
class EventLoopThreadPool : NonCopyable
{
  public:
    EventLoopThreadPool() = delete;
    EventLoopThreadPool(size_t threadNum);
    void start();
    //void stop();
    void wait();
    size_t getLoopNum() { return loopThreadVector_.size(); }
    EventLoop *getNextLoop();

  private:
    std::vector<EventLoopThread> loopThreadVector_;
    size_t loopIndex_;
};
} // namespace trantor