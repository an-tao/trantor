/**
 *
 *  ObjectPool.h
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
#include <vector>
#include <memory>
#include <type_traits>
#include <mutex>

namespace trantor
{
template <typename T>
class ObjectPool : public NonCopyable, public std::enable_shared_from_this<ObjectPool<T>>
{
  public:
    std::shared_ptr<T> getObject()
    {
        static_assert(!std::is_pointer<T>::value, "The parameter type of the ObjectPool template can't be pointer type");
        T *p = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_objs.empty())
            {
                p = _objs.back();
                _objs.pop_back();
            }
        }

        if (p == nullptr)
        {
            p = new T;
        }

        assert(p);
        std::weak_ptr<ObjectPool<T>> weakPtr = this->shared_from_this();
        auto obj = std::shared_ptr<T>(p, [weakPtr](T *ptr) {
            auto self = weakPtr.lock();
            if (self)
            {
                std::lock_guard<std::mutex> lock(self->_mtx);
                self->_objs.push_back(ptr);
            }
            else
            {
                delete ptr;
            }
        });
        return obj;
    }

  private:
    std::vector<T *> _objs;
    std::mutex _mtx;
};
} // namespace trantor