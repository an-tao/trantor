/**
 *
 *  SerialTaskQueue.h
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

#include "TaskQueue.h"
#include <trantor/net/EventLoopThread.h>
#include <trantor/exports.h>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
namespace trantor
{
/**
 * @brief This class represents a task queue in which all tasks are executed one
 * by one.
 *
 */
class SerialTaskQueue : public TaskQueue
{
  public:
    /**
     * @brief Run a task in the queue.
     *
     * @param task
     */
    TRANTOR_EXPORT virtual void runTaskInQueue(
        const std::function<void()> &task);
    TRANTOR_EXPORT virtual void runTaskInQueue(std::function<void()> &&task);

    /**
     * @brief Get the name of the queue.
     *
     * @return std::string
     */
    virtual std::string getName() const
    {
        return queueName_;
    };

    /**
     * @brief Wait until all tasks in the queue are finished.
     *
     */
    TRANTOR_EXPORT void waitAllTasksFinished();

    SerialTaskQueue() = delete;

    /**
     * @brief Construct a new serail task queue instance.
     *
     * @param name
     */
    TRANTOR_EXPORT explicit SerialTaskQueue(const std::string &name);

    TRANTOR_EXPORT virtual ~SerialTaskQueue();

    /**
     * @brief Check whether a task is running in the queue.
     *
     * @return true
     * @return false
     */
    bool isRuningTask()
    {
        return loopThread_.getLoop()
                   ? loopThread_.getLoop()->isCallingFunctions()
                   : false;
    }

    /**
     * @brief Get the number of tasks in the queue.
     *
     * @return size_t
     */
    TRANTOR_EXPORT size_t getTaskCount();

    /**
     * @brief Stop the queue.
     *
     */
    TRANTOR_EXPORT void stop();

  protected:
    std::string queueName_;
    EventLoopThread loopThread_;
    bool stop_{false};
};
}  // namespace trantor
