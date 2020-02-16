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
#include <memory>

namespace trantor
{
class EventLoopThreadPool : NonCopyable
{
  public:
    EventLoopThreadPool() = delete;
    EventLoopThreadPool(size_t threadNum,
                        const std::string &name = "EventLoopThreadPool");
    void start();
    // void stop();
    void wait();
    size_t size()
    {
        return loopThreadVector_.size();
    }
    EventLoop *getNextLoop();
    EventLoop *getLoop(size_t id);
    std::vector<EventLoop *> getLoops() const;

  private:
    std::vector<std::shared_ptr<EventLoopThread>> loopThreadVector_;
    size_t loopIndex_;
};
}  // namespace trantor
