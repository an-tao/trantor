/**
 *
 *  Acceptor.h
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
#include <trantor/utils/NonCopyable.h>
#include "Socket.h"
#include <trantor/net/InetAddress.h>
#include "Channel.h"
#include <functional>

namespace trantor
{
typedef std::function<void(int fd, const InetAddress &)> NewConnectionCallback;
class Acceptor : NonCopyable
{
  public:
    Acceptor(EventLoop *loop, const InetAddress &addr);
    ~Acceptor();
    const InetAddress &addr() const { return _addr; }
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        _newConnectionCallback = cb;
    };
    void listen();

  protected:
    Socket _sock;
    InetAddress _addr;
    EventLoop *_loop;
    NewConnectionCallback _newConnectionCallback;
    Channel _acceptChannel;
    int _idleFd;
    void readCallback();
};
} // namespace trantor
