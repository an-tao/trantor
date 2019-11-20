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
#include <vector>

namespace trantor
{
constexpr size_t kResolveBufferLength{16 * 1024};
class NormalResolver : public Resolver,
                       public NonCopyable,
                       public std::enable_shared_from_this<NormalResolver>
{
  public:
    virtual void resolve(const std::string& hostname,
                         const Callback& callback) override;
    explicit NormalResolver(size_t timeout)
        : timeout_(timeout), resolveBuffer_(kResolveBufferLength)
    {
    }
    virtual ~NormalResolver()
    {
    }

  private:
    static std::unordered_map<std::string,
                              std::pair<struct in_addr, trantor::Date>>&
    globalCache()
    {
        static std::unordered_map<std::string,
                                  std::pair<struct in_addr, trantor::Date>>
            dnsCache_;
        return dnsCache_;
    }
    static std::mutex& globalMutex()
    {
        static std::mutex mutex_;
        return mutex_;
    }
    trantor::SerialTaskQueue taskQueue_{"Dns Queue"};
    const size_t timeout_;
    std::vector<char> resolveBuffer_;
};
}  // namespace trantor