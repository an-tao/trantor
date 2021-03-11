/**
 *
 *  @file EventLoopThreadPool.h
 *  @author An Tao
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
#include <trantor/exports.h>
#include <vector>
#include <memory>

namespace trantor
{
/**
 * @brief This class represents a pool of EventLoopThread objects
 *
 */
class EventLoopThreadPool : NonCopyable
{
  public:
    EventLoopThreadPool() = delete;

    /**
     * @brief Construct a new event loop thread pool instance.
     *
     * @param threadNum The number of threads
     * @param name The name of the EventLoopThreadPool object.
     */
    TRANTOR_EXPORT EventLoopThreadPool(
        size_t threadNum,
        const std::string &name = "EventLoopThreadPool");

    /**
     * @brief Run all event loops in the pool.
     * @note This function doesn't block the current thread.
     */
    TRANTOR_EXPORT void start();

    /**
     * @brief Wait for all event loops in the pool to quit.
     *
     * @note This function blocks the current thread.
     */
    TRANTOR_EXPORT void wait();

    /**
     * @brief Return the number of the event loop.
     *
     * @return size_t
     */
    size_t size()
    {
        return loopThreadVector_.size();
    }

    /**
     * @brief Get the next event loop in the pool.
     *
     * @return EventLoop*
     */
    TRANTOR_EXPORT EventLoop *getNextLoop();

    /**
     * @brief Get the event loop in the `id` position in the pool.
     *
     * @param id The id of the first event loop is zero. If the id >= the number
     * of event loops, nullptr is returned.
     * @return EventLoop*
     */
    TRANTOR_EXPORT EventLoop *getLoop(size_t id);

    /**
     * @brief Get all event loops in the pool.
     *
     * @return std::vector<EventLoop *>
     */
    TRANTOR_EXPORT std::vector<EventLoop *> getLoops() const;

  private:
    std::vector<std::shared_ptr<EventLoopThread>> loopThreadVector_;
    size_t loopIndex_;
};
}  // namespace trantor
