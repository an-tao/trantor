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

#include <trantor/utils/config.h>
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
#ifdef USE_OPENSSL

#include <openssl/ssl.h>
#include <openssl/err.h>

#endif
namespace trantor
{

class Acceptor;

class TcpServer : NonCopyable
{
  public:
    TcpServer(EventLoop *loop, const InetAddress &address, const std::string &name);
    ~TcpServer();
    void start();
    void setIoLoopNum(size_t num);
    void setRecvMessageCallback(const RecvMessageCallback &cb) { recvMessageCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setRecvMessageCallback(RecvMessageCallback &&cb) { recvMessageCallback_ = std::move(cb); }
    void setConnectionCallback(ConnectionCallback &&cb) { connectionCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback &&cb) { writeCompleteCallback_ = std::move(cb); }
    const std::string &name() const { return serverName_; }
    const std::string ipPort() const;
    EventLoop *getLoop() const { return loop_; }

    //An idle connection is a connection that has no read or write,
    //kick off it after timeout ;
    void kickoffIdleConnections(size_t timeout)
    {
        loop_->runInLoop([=]() {
            assert(!_started);
            _idleTimeout = timeout;
        });
    }

#ifdef USE_OPENSSL
    void enableSSL(const std::string &certPath, const std::string &keyPath);
#endif
  private:
    EventLoop *loop_;
    std::unique_ptr<Acceptor> acceptorPtr_;
    void newConnection(int fd, const InetAddress &peer);
    std::string serverName_;
    std::set<TcpConnectionPtr> connSet_;

    RecvMessageCallback recvMessageCallback_;
    ConnectionCallback connectionCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    size_t _idleTimeout = 0;
    std::map<EventLoop *, std::shared_ptr<TimingWheel>> _timeingWheelMap;
    void connectionClosed(const TcpConnectionPtr &connectionPtr);
    std::unique_ptr<EventLoopThreadPool> loopPoolPtr_;
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

#ifdef USE_OPENSSL
    //OpenSSL SSL context Object;
    std::shared_ptr<SSL_CTX> _sslCtxPtr;
#endif
};

} // namespace trantor
