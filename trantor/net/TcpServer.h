/**
 *
 *  TcpServer.h
 *  An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/trantor
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *  Trantor
 *
 */

#pragma once
#include <trantor/net/callbacks.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoopThreadPool.h>
#include <trantor/net/InetAddress.h>
#include <trantor/net/TcpConnection.h>
#include <trantor/utils/TimingWheel.h>
#include <string>
#include <memory>
#include <set>
#include <signal.h>
namespace trantor
{
class Acceptor;
class SSLContext;

class TcpServer : NonCopyable
{
  public:
    TcpServer(EventLoop *loop,
              const InetAddress &address,
              const std::string &name,
              bool reUseAddr = true,
              bool reUsePort = true);
    ~TcpServer();
    void start();
    void setIoLoopNum(size_t num)
    {
        assert(!_started);
        _loopPoolPtr = std::make_shared<EventLoopThreadPool>(num);
        _loopPoolPtr->start();
    }
    void setIoLoopThreadPool(const std::shared_ptr<EventLoopThreadPool> &pool)
    {
        assert(pool->getLoopNum() > 0);
        assert(!_started);
        _loopPoolPtr = pool;
        _loopPoolPtr->start();
    }
    void setRecvMessageCallback(const RecvMessageCallback &cb)
    {
        _recvMessageCallback = cb;
    }
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        _connectionCallback = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        _writeCompleteCallback = cb;
    }

    void setRecvMessageCallback(RecvMessageCallback &&cb)
    {
        _recvMessageCallback = std::move(cb);
    }
    void setConnectionCallback(ConnectionCallback &&cb)
    {
        _connectionCallback = std::move(cb);
    }
    void setWriteCompleteCallback(WriteCompleteCallback &&cb)
    {
        _writeCompleteCallback = std::move(cb);
    }
    const std::string &name() const
    {
        return _serverName;
    }
    const std::string ipPort() const;
    EventLoop *getLoop() const
    {
        return _loop;
    }
    std::vector<EventLoop *> getIoLoops() const
    {
        return _loopPoolPtr->getLoops();
    }
    // An idle connection is a connection that has no read or write,
    // kick off it after timeout ;
    void kickoffIdleConnections(size_t timeout)
    {
        _loop->runInLoop([=]() {
            assert(!_started);
            _idleTimeout = timeout;
        });
    }

    void enableSSL(const std::string &certPath, const std::string &keyPath);

  private:
    EventLoop *_loop;
    std::unique_ptr<Acceptor> _acceptorPtr;
    void newConnection(int fd, const InetAddress &peer);
    std::string _serverName;
    std::set<TcpConnectionPtr> _connSet;

    RecvMessageCallback _recvMessageCallback;
    ConnectionCallback _connectionCallback;
    WriteCompleteCallback _writeCompleteCallback;

    size_t _idleTimeout = 0;
    std::map<EventLoop *, std::shared_ptr<TimingWheel>> _timingWheelMap;
    void connectionClosed(const TcpConnectionPtr &connectionPtr);
    std::shared_ptr<EventLoopThreadPool> _loopPoolPtr;
    class IgnoreSigPipe
    {
      public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            LOG_TRACE << "Ignore SIGPIPE";
        }
    };

    IgnoreSigPipe initObj;

    bool _started = false;

    // OpenSSL SSL context Object;
    std::shared_ptr<SSLContext> _sslCtxPtr;
};

}  // namespace trantor
