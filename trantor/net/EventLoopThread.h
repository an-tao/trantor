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
    EventLoop *getLoop() const { return _loop; }
    void run();

  private:
    EventLoop *_loop;
    std::string _loopThreadName;
    void loopFuncs();
    std::thread _thread;
    std::promise<EventLoop *> _promiseForLoopPointer;
    std::promise<int> _promiseForRun;
    std::promise<int> _promiseForLoop;
    std::once_flag _once;
};

} // namespace trantor
