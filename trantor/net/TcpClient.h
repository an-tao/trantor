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
#include <trantor/utils/config.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/net/TcpConnection.h>
#include <functional>
#include <thread>
#include <atomic>
#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
namespace trantor
{
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : NonCopyable
{
  public:
    // TcpClient(EventLoop* loop);
    // TcpClient(EventLoop* loop, const string& host, uint16_t port);
    TcpClient(EventLoop *loop,
              const InetAddress &serverAddr,
              const std::string &nameArg);
    ~TcpClient(); // force out-line dtor, for scoped_ptr members.

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _connection;
    }

    EventLoop *getLoop() const { return _loop; }
    bool retry() const { return _retry; }
    void enableRetry() { _retry = true; }

    const std::string &name() const
    {
        return _name;
    }

    /// Set connection callback.
    /// Not thread safe.
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        _connectionCallback = cb;
    }
    void setConnectionErrorCallback(const ConnectionErrorCallback &cb)
    {
        _connectionErrorCallback = cb;
    }
    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const RecvMessageCallback &cb)
    {
        _messageCallback = cb;
    }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        _writeCompleteCallback = cb;
    }

    void setConnectionCallback(ConnectionCallback &&cb)
    {
        _connectionCallback = std::move(cb);
    }
    void setMessageCallback(RecvMessageCallback &&cb)
    {
        _messageCallback = std::move(cb);
    }
    void setWriteCompleteCallback(WriteCompleteCallback &&cb)
    {
        _writeCompleteCallback = std::move(cb);
    }
#ifdef USE_OPENSSL
    void enableSSL();
#endif

  private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd);
    /// Not thread safe, but in loop
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *_loop;
    ConnectorPtr _connector; // avoid revealing Connector
    const std::string _name;
    ConnectionCallback _connectionCallback;
    ConnectionErrorCallback _connectionErrorCallback;
    RecvMessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;
    std::atomic_bool _retry;   // atomic
    std::atomic_bool _connect; // atomic
    // always in loop thread
    int _nextConnId;
    mutable std::mutex _mutex;
    TcpConnectionPtr _connection; // @GuardedBy _mutex
#ifdef USE_OPENSSL
    std::shared_ptr<SSL_CTX> _sslCtxPtr;
#endif
};

} // namespace trantor
