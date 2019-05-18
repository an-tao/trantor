/**
 *
 *  TimingWheel.h
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
#include <trantor/utils/Logger.h>
#include <map>
#include <mutex>
#include <deque>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <assert.h>

#define TIMING_BUCKET_NUM_PER_WHEEL 100
#define TIMING_TICK_INTERVAL 1.0

// Four wheels with 200 buckets per wheel means the cache map can work with
// a timeout up to 200^4 seconds,about 50 years;

namespace trantor
{
typedef std::shared_ptr<void> EntryPtr;

typedef std::unordered_set<EntryPtr> EntryBucket;
typedef std::deque<EntryBucket> BucketQueue;

class TimingWheel
{
  public:
    class CallbackEntry
    {
      public:
        CallbackEntry(std::function<void()> cb) : cb_(std::move(cb))
        {
        }
        ~CallbackEntry()
        {
            cb_();
        }

      private:
        std::function<void()> cb_;
    };

    /// constructor
    /// @param loop
    /// eventloop pointer
    /// @param tickInterval
    /// second
    /// @param wheelsNum
    /// number of wheels
    /// @param bucketsNumPerWheel
    /// buckets number per wheel
    /// The max delay of the CacheMap is about
    /// tickInterval*(bucketsNumPerWheel^wheelsNum) seconds.

    TimingWheel(trantor::EventLoop *loop,
                size_t maxTimeout,
                float tickInterval = TIMING_TICK_INTERVAL,
                size_t bucketsNumPerWheel = TIMING_BUCKET_NUM_PER_WHEEL)
        : _loop(loop),
          _tickInterval(tickInterval),
          _bucketsNumPerWheel(bucketsNumPerWheel)
    {
        assert(maxTimeout > 1);
        assert(tickInterval > 0);
        size_t maxTickNum = maxTimeout / tickInterval;
        _wheelsNum = 1;
        while (maxTickNum > _bucketsNumPerWheel)
        {
            _wheelsNum++;
            maxTickNum = maxTickNum / _bucketsNumPerWheel;
        }
        _wheels.resize(_wheelsNum);
        for (size_t i = 0; i < _wheelsNum; i++)
        {
            _wheels[i].resize(_bucketsNumPerWheel);
        }
        _timerId = _loop->runEvery(_tickInterval, [=]() {
            _ticksCounter++;
            size_t t = _ticksCounter;
            size_t pow = 1;
            for (size_t i = 0; i < _wheelsNum; i++)
            {
                if ((t % pow) == 0)
                {
                    EntryBucket tmp;
                    {
                        // use tmp val to make this critical area as short as
                        // possible.
                        _wheels[i].front().swap(tmp);
                        _wheels[i].pop_front();
                        _wheels[i].push_back(EntryBucket());
                    }
                }
                pow = pow * _bucketsNumPerWheel;
            }
        });
    };
    ~TimingWheel()
    {
        _loop->invalidateTimer(_timerId);

        for (int i = _wheels.size() - 1; i >= 0; i--)
        {
            _wheels[i].clear();
        }

        LOG_TRACE << "TimingWheel destruct!";
    }

    // If timeout>0,the value will be erased
    // within the 'timeout' seconds after the last access
    void insertEntry(size_t delay, EntryPtr entryPtr)
    {
        if (delay <= 0)
            return;
        if (!entryPtr)
            return;
        if (_loop->isInLoopThread())
        {
            insertEntryInloop(delay, entryPtr);
        }
        else
        {
            _loop->runInLoop([=]() { insertEntryInloop(delay, entryPtr); });
        }
    }

    void insertEntryInloop(size_t delay, EntryPtr entryPtr)
    {
        // protected by bucketMutex;
        _loop->assertInLoopThread();

        delay = delay / _tickInterval + 1;
        size_t t = _ticksCounter;
        for (size_t i = 0; i < _wheelsNum; i++)
        {
            if (delay <= _bucketsNumPerWheel)
            {
                _wheels[i][delay - 1].insert(entryPtr);
                break;
            }
            if (i < (_wheelsNum - 1))
            {
                entryPtr = std::make_shared<CallbackEntry>([=]() {
                    if (delay > 0)
                    {
                        _wheels[i][(delay + (t % _bucketsNumPerWheel) - 1) %
                                   _bucketsNumPerWheel]
                            .insert(entryPtr);
                    }
                });
            }
            else
            {
                // delay is too long to put entry at valid position in wheels;
                _wheels[i][_bucketsNumPerWheel - 1].insert(entryPtr);
            }
            delay =
                (delay + (t % _bucketsNumPerWheel) - 1) / _bucketsNumPerWheel;
            t = t / _bucketsNumPerWheel;
        }
    }

    EventLoop *getLoop()
    {
        return _loop;
    }

  private:
    std::vector<BucketQueue> _wheels;

    std::atomic<size_t> _ticksCounter = ATOMIC_VAR_INIT(0);

    trantor::TimerId _timerId;
    trantor::EventLoop *_loop;

    float _tickInterval;
    size_t _wheelsNum;
    size_t _bucketsNumPerWheel;
};
}  // namespace trantor
