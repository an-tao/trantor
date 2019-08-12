// Copyright 2016, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#pragma once
#include <trantor/net/Resolver.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/SerialTaskQueue.h>
#include <memory>

namespace trantor
{
class NormalResolver : public Resolver,
                       public NonCopyable,
                       public std::enable_shared_from_this<NormalResolver>
{
  public:
    virtual void resolve(const std::string& hostname,
                         const Callback& callback) override;
    NormalResolver(size_t timeout) : _taskQueue("Dns Queue"), _timeout(timeout)
    {
    }
    virtual ~NormalResolver()
    {
    }

  private:
#ifdef __linux__
    static __thread char t_resolveBuffer[64 * 1024];
#endif
    std::mutex _dnsMutex;
    std::unordered_map<std::string, std::pair<struct in_addr, trantor::Date>>
        _dnsCache;
    trantor::SerialTaskQueue _taskQueue;
    const size_t _timeout = 60;
};
}  // namespace trantor