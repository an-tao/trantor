/**
 *
 *  LockFreeQueue.h
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
#include <trantor/utils/NonCopyable.h>
#include <atomic>
#include <type_traits>
#include <assert.h>
namespace trantor
{
///A lock-free multiple producers single consumer queue
template <typename T>
class MpscQueue : public NonCopyable
{
  public:
    MpscQueue() : _head(new BufferNode),
                  _tail(_head.load(std::memory_order_relaxed))
    {
        BufferNode *front = _head.load(std::memory_order_relaxed);
        front->next.store(NULL, std::memory_order_relaxed);
    }

    ~MpscQueue()
    {
        T output;
        while (this->dequeue(output))
        {
        }
        BufferNode *front = _head.load(std::memory_order_relaxed);
        delete front;
    }

    void enqueue(T &&input)
    {
        BufferNode *node = new BufferNode;
        node->data = std::move(input);
        node->next.store(NULL, std::memory_order_relaxed);

        BufferNode *prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    void enqueue(const T &input)
    {
        BufferNode *node = new BufferNode;
        node->data = input;
        node->next.store(NULL, std::memory_order_relaxed);

        BufferNode *prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    bool dequeue(T &output)
    {
        BufferNode *tail = _tail.load(std::memory_order_relaxed);
        BufferNode *next = tail->next.load(std::memory_order_acquire);

        if (next == NULL)
        {
            return false;
        }
#ifdef __linux__
        output = std::move(next->data);
#else
        output = next->data;
        /// Immediately destroy some RAII objects in the next->data
        next->data = T();
#endif
        _tail.store(next, std::memory_order_release);
        delete tail;
        return true;
    }

  private:
    struct BufferNode
    {
        T data;
        std::atomic<BufferNode *> next;
    };

    std::atomic<BufferNode *> _head;
    std::atomic<BufferNode *> _tail;
};

} // namespace trantor
