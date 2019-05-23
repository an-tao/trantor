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
                size_t bucketsNumPerWheel = TIMING_BUCKET_NUM_PER_WHEEL);

    // If timeout>0, the value will be erased within the 'timeout' seconds after
    // the last access
    void insertEntry(size_t delay, EntryPtr entryPtr);

    void insertEntryInloop(size_t delay, EntryPtr entryPtr);

    EventLoop *getLoop()
    {
        return _loop;
    }

    ~TimingWheel();

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
