/**
 *
 *  EventLoopThreadPool.cc
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

#include <trantor/net/EventLoopThreadPool.h>
using namespace trantor;
EventLoopThreadPool::EventLoopThreadPool(size_t threadNum,
                                         const std::string &name)
    : _loopIndex(0)
{
    for (size_t i = 0; i < threadNum; i++)
    {
        _loopThreadVector.emplace_back(std::make_shared<EventLoopThread>(name));
    }
}
void EventLoopThreadPool::start()
{
    for (unsigned int i = 0; i < _loopThreadVector.size(); i++)
    {
        _loopThreadVector[i]->run();
    }
}
// void EventLoopThreadPool::stop(){
//    for(unsigned int i=0;i<_loopThreadVector.size();i++)
//    {
//        _loopThreadVector[i].stop();
//    }
//}
void EventLoopThreadPool::wait()
{
    for (unsigned int i = 0; i < _loopThreadVector.size(); i++)
    {
        _loopThreadVector[i]->wait();
    }
}
EventLoop *EventLoopThreadPool::getNextLoop()
{
    if (_loopThreadVector.size() > 0)
    {
        EventLoop *loop = _loopThreadVector[_loopIndex]->getLoop();
        _loopIndex++;
        if (_loopIndex >= _loopThreadVector.size())
            _loopIndex = 0;
        return loop;
    }
    return NULL;
}

std::vector<EventLoop *> EventLoopThreadPool::getLoops() const
{
    std::vector<EventLoop *> ret;
    for (auto &loopThread : _loopThreadVector)
    {
        ret.push_back(loopThread->getLoop());
    }
    return ret;
}