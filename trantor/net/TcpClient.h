// taken from muduo
// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

// Copyright 2016, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#pragma once
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/net/TcpConnection.h>
#include <functional>
#include <thread>
#include <atomic>
#include <signal.h>
namespace trantor
{
class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;
class SSLContext;
class TcpClient : NonCopyable
{
  public:
    // TcpClient(EventLoop* loop);
    // TcpClient(EventLoop* loop, const string& host, uint16_t port);
    TcpClient(EventLoop *loop,
              const InetAddress &serverAddr,
              const std::string &nameArg);
    ~TcpClient();  // force out-line dtor, for scoped_ptr members.

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    EventLoop *getLoop() const
    {
        return loop_;
    }
    bool retry() const
    {
        return retry_;
    }
    void enableRetry()
    {
        retry_ = true;
    }

    const std::string &name() const
    {
        return name_;
    }

    /// Set connection callback.
    /// Not thread safe.
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setConnectionErrorCallback(const ConnectionErrorCallback &cb)
    {
        connectionErrorCallback_ = cb;
    }
    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const RecvMessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setConnectionCallback(ConnectionCallback &&cb)
    {
        connectionCallback_ = std::move(cb);
    }
    void setMessageCallback(RecvMessageCallback &&cb)
    {
        messageCallback_ = std::move(cb);
    }
    void setWriteCompleteCallback(WriteCompleteCallback &&cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }
    void enableSSL();

  private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd);
    /// Not thread safe, but in loop
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_;  // avoid revealing Connector
    const std::string name_;
    ConnectionCallback connectionCallback_;
    ConnectionErrorCallback connectionErrorCallback_;
    RecvMessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    std::atomic_bool retry_;    // atomic
    std::atomic_bool connect_;  // atomic
    // always in loop thread
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_;  // @GuardedBy mutex_
    std::shared_ptr<SSLContext> sslCtxPtr_;
#ifndef _WIN32
    class IgnoreSigPipe
    {
      public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
        }
    };

    static IgnoreSigPipe initObj;
#endif
};

}  // namespace trantor
