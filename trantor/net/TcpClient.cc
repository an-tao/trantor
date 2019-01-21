// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

// Taken from muduo and modified by an tao


#include <trantor/net/TcpClient.h>

#include <trantor/utils/Logger.h>
#ifdef USE_OPENSSL
#include "ssl/SSLConnection.h"
#endif
#include "Connector.h"
#include "inner/TcpConnectionImpl.h"
#include <trantor/net/EventLoop.h>
#include "Socket.h"

#include <stdio.h> // snprintf
#include <functional>

using namespace trantor;
using namespace std::placeholders;

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace trantor
{

void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop(std::bind(&TcpConnectionImpl::connectDestroyed,
                                std::dynamic_pointer_cast<TcpConnectionImpl>(conn)));
}

void removeConnector(const ConnectorPtr &connector)
{
    //connector->
}
static void defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddr().toIpPort() << " -> "
              << conn->peerAddr().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

static void defaultMessageCallback(const TcpConnectionPtr &,
                                   MsgBuffer *buf)
{
    buf->retrieveAll();
}

} // namespace trantor

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr, false)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    connector_->setErrorCallback([=]() {
        if (_connectionErrorCallback)
        {
            _connectionErrorCallback();
        }
    });
    LOG_TRACE << "TcpClient::TcpClient[" << name_
              << "] - connector ";
}

TcpClient::~TcpClient()
{
    LOG_TRACE << "TcpClient::~TcpClient[" << name_
              << "] - connector ";
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        // TODO: not 100% safe, if we are in different thread
        CloseCallback cb = std::bind(&trantor::removeConnection, loop_, _1);
        loop_->runInLoop(
            std::bind(&TcpConnectionImpl::setCloseCallback,
                      std::dynamic_pointer_cast<TcpConnectionImpl>(conn), cb));
        if (unique)
        {
            conn->forceClose();
        }
    }
    else
    {
        connector_->stop();
        loop_->runAfter(1, [=]() {
            trantor::removeConnector(connector_);
        });
    }
}

void TcpClient::connect()
{
    // TODO: check state
    LOG_TRACE << "TcpClient::connect[" << name_ << "] - connecting to "
              << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(Socket::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    // TODO poll with zero timeout to double confirm the new connection
    // TODO use make_shared if necessary
#ifdef USE_OPENSSL
    std::shared_ptr<TcpConnectionImpl> conn;
    if (_sslCtxPtr)
    {
        conn = std::make_shared<SSLConnection>(loop_,
                                               sockfd,
                                               localAddr,
                                               peerAddr,
                                               _sslCtxPtr,
                                               false);
    }
    else
    {
        conn = std::make_shared<TcpConnectionImpl>(loop_,
                                                   sockfd,
                                                   localAddr,
                                                   peerAddr);
    }
#else
    auto conn = std::make_shared<TcpConnectionImpl>(loop_,
                                                    sockfd,
                                                    localAddr,
                                                    peerAddr);
#endif
    conn->setConnectionCallback(connectionCallback_);
    conn->setRecvMsgCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, _1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnectionImpl::connectDestroyed,
                                 std::dynamic_pointer_cast<TcpConnectionImpl>(conn)));
    if (retry_ && connect_)
    {
        LOG_TRACE << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                  << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}

#ifdef USE_OPENSSL
void TcpClient::enableSSL()
{
    //init OpenSSL
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || \
    (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
    // Initialize OpenSSL once;
    static std::once_flag once;
    std::call_once(once, []() {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    });
#endif
    /* Create a new OpenSSL context */
    _sslCtxPtr = std::shared_ptr<SSL_CTX>(
        SSL_CTX_new(SSLv23_method()),
        [](SSL_CTX *ctx) {
            SSL_CTX_free(ctx);
        });
    assert(_sslCtxPtr);
}
#endif
