/**
 *
 *  EventLoopThread.h
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

#include <trantor/net/EventLoop.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/SerialTaskQueue.h>
#include <mutex>
namespace trantor
{
class EventLoopThread : NonCopyable
{
  public:
    EventLoopThread();
    ~EventLoopThread();
    void run();
    //void stop();
    void wait();
    EventLoop *getLoop() { return loop_; }

  private:
    EventLoop *loop_;
    SerialTaskQueue loopQueue_;
    void loopFuncs();
    std::condition_variable cond_;
    std::mutex mutex_;
};
} // namespace trantor
