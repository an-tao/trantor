/**
 *
 *  TimingWheel.cc
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

#include <trantor/utils/TimingWheel.h>

using namespace trantor;

TimingWheel::TimingWheel(trantor::EventLoop *loop,
                         size_t maxTimeout,
                         float tickInterval,
                         size_t bucketsNumPerWheel)
    : loop_(loop),
      tickInterval_(tickInterval),
      bucketsNumPerWheel_(bucketsNumPerWheel)
{
    assert(maxTimeout > 1);
    assert(tickInterval > 0);
    size_t maxTickNum = maxTimeout / tickInterval;
    wheelsNum_ = 1;
    while (maxTickNum > bucketsNumPerWheel_)
    {
        wheelsNum_++;
        maxTickNum = maxTickNum / bucketsNumPerWheel_;
    }
    wheels_.resize(wheelsNum_);
    for (size_t i = 0; i < wheelsNum_; i++)
    {
        wheels_[i].resize(bucketsNumPerWheel_);
    }
    timerId_ = loop_->runEvery(tickInterval_, [=]() {
        ticksCounter_++;
        size_t t = ticksCounter_;
        size_t pow = 1;
        for (size_t i = 0; i < wheelsNum_; i++)
        {
            if ((t % pow) == 0)
            {
                EntryBucket tmp;
                {
                    // use tmp val to make this critical area as short as
                    // possible.
                    wheels_[i].front().swap(tmp);
                    wheels_[i].pop_front();
                    wheels_[i].push_back(EntryBucket());
                }
            }
            pow = pow * bucketsNumPerWheel_;
        }
    });
}

TimingWheel::~TimingWheel()
{
    loop_->invalidateTimer(timerId_);

    for (int i = wheels_.size() - 1; i >= 0; i--)
    {
        wheels_[i].clear();
    }

    LOG_TRACE << "TimingWheel destruct!";
}

void TimingWheel::insertEntry(size_t delay, EntryPtr entryPtr)
{
    if (delay <= 0)
        return;
    if (!entryPtr)
        return;
    if (loop_->isInLoopThread())
    {
        insertEntryInloop(delay, entryPtr);
    }
    else
    {
        loop_->runInLoop([=]() { insertEntryInloop(delay, entryPtr); });
    }
}

void TimingWheel::insertEntryInloop(size_t delay, EntryPtr entryPtr)
{
    // protected by bucketMutex;
    loop_->assertInLoopThread();

    delay = delay / tickInterval_ + 1;
    size_t t = ticksCounter_;
    for (size_t i = 0; i < wheelsNum_; i++)
    {
        if (delay <= bucketsNumPerWheel_)
        {
            wheels_[i][delay - 1].insert(entryPtr);
            break;
        }
        if (i < (wheelsNum_ - 1))
        {
            entryPtr = std::make_shared<CallbackEntry>([=]() {
                if (delay > 0)
                {
                    wheels_[i][(delay + (t % bucketsNumPerWheel_) - 1) %
                               bucketsNumPerWheel_]
                        .insert(entryPtr);
                }
            });
        }
        else
        {
            // delay is too long to put entry at valid position in wheels;
            wheels_[i][bucketsNumPerWheel_ - 1].insert(entryPtr);
        }
        delay = (delay + (t % bucketsNumPerWheel_) - 1) / bucketsNumPerWheel_;
        t = t / bucketsNumPerWheel_;
    }
}