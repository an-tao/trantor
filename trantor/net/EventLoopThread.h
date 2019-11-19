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
#include <mutex>
#include <thread>
#include <memory>
#include <condition_variable>
#include <future>

namespace trantor
{
class EventLoopThread : NonCopyable
{
  public:
    explicit EventLoopThread(const std::string &threadName = "EventLoopThread");
    ~EventLoopThread();
    void wait();
    EventLoop *getLoop() const
    {
        return loop_;
    }
    void run();

  private:
    EventLoop *loop_;
    std::string loopThreadName_;
    void loopFuncs();
    std::promise<EventLoop *> promiseForLoopPointer_;
    std::promise<int> promiseForRun_;
    std::promise<int> promiseForLoop_;
    std::once_flag once_;
    std::thread thread_;
};

}  // namespace trantor
