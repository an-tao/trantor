// Copyright 2016, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#pragma once
#include <memory>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>

namespace trantor
{
class Resolver
{
  public:
    using Callback = std::function<void(const trantor::InetAddress&)>;
    static std::shared_ptr<Resolver> newResolver(EventLoop* loop = nullptr,
                                                 size_t timeout = 60);
    virtual void resolve(const std::string& hostname,
                         const Callback& callback) = 0;
    virtual ~Resolver()
    {
    }
    static bool isCAresUsed();
};
}  // namespace trantor