/**
 *
 *  SerialTaskQueue.cc
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

#include <trantor/utils/SerialTaskQueue.h>
#include <trantor/utils/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
namespace trantor
{
SerialTaskQueue::SerialTaskQueue(const std::string &name)
    : _queueName(name.empty() ? "SerailTaskQueue" : name),
      _loopThread(_queueName)
{
    _loopThread.run();
}
void SerialTaskQueue::stop()
{
    _stop = true;
    _loopThread.getLoop()->quit();
    _loopThread.wait();
}
SerialTaskQueue::~SerialTaskQueue()
{
    if (!_stop)
        stop();
    LOG_TRACE << "destruct SerialTaskQueue('" << _queueName << "')";
}
void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task)
{
    _loopThread.getLoop()->runInLoop(task);
}
void SerialTaskQueue::runTaskInQueue(std::function<void()> &&task)
{
    _loopThread.getLoop()->runInLoop(std::move(task));
}

void SerialTaskQueue::waitAllTasksFinished()
{
    syncTaskInQueue([]() {

    });
}

}; // namespace trantor
