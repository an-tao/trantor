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
#include "Connector.h"
#include "inner/TcpConnectionImpl.h"
#include <trantor/net/EventLoop.h>

#include <functional>
#include <algorithm>
#include <atomic>
#include <memory>

#include "Socket.h"

#include <stdio.h>  // snprintf

using namespace trantor;
using namespace std::placeholders;

namespace trantor
{
// void removeConnector(const ConnectorPtr &)
// {
//     // connector->
// }
#ifndef _WIN32
TcpClient::IgnoreSigPipe TcpClient::initObj;
#endif

static void defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddr().toIpPort() << " -> "
              << conn->peerAddr().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register
    // message callback only.
}

static void defaultMessageCallback(const TcpConnectionPtr &, MsgBuffer *buf)
{
    buf->retrieveAll();
}

}  // namespace trantor

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr, false)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true)
{
    (void)validateCert_;
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    connector_->setErrorCallback([this]() {
        if (connectionErrorCallback_)
        {
            connectionErrorCallback_();
        }
    });
    LOG_TRACE << "TcpClient::TcpClient[" << name_ << "] - connector ";
}

TcpClient::~TcpClient()
{
    LOG_TRACE << "TcpClient::~TcpClient[" << name_ << "] - connector ";
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_ == nullptr)
        return;
    assert(loop_ == connection_->getLoop());
    auto conn =
        std::atomic_load_explicit(&connection_, std::memory_order_relaxed);
    loop_->runInLoop([conn = std::move(conn), loop = loop_]() {
        conn->setCloseCallback(
            [loop, conn = std::move(conn)](const TcpConnectionPtr &connPtr) {
                loop->queueInLoop([connPtr]() { connPtr->connectDestroyed(); });
            });
    });
    connection_->forceClose();
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
    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    // TODO poll with zero timeout to double confirm the new connection
    // TODO use make_shared if necessary
    TcpConnectionPtr conn;
    LOG_TRACE << "SSL enabled: " << (sslPolicyPtr_ ? "true" : "false");
    std::weak_ptr<TlsInitiator> weakInitiator =
        std::shared_ptr<TlsInitiator>(shared_from_this());
    if (sslPolicyPtr_)
    {
        assert(sslContextPtr_);
        conn =
            newTLSConnection(std::make_shared<TcpConnectionImpl>(loop_,
                                                                 sockfd,
                                                                 localAddr,
                                                                 peerAddr,
                                                                 weakInitiator),
                             sslPolicyPtr_,
                             sslContextPtr_);
    }
    else
    {
        conn = std::make_shared<TcpConnectionImpl>(
            loop_, sockfd, localAddr, peerAddr, weakInitiator);
    }
    conn->setConnectionCallback(connectionCallback_);
    conn->setRecvMsgCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    std::weak_ptr<TcpClient> weakSelf(shared_from_this());
    auto closeCb = std::function<void(const TcpConnectionPtr &)>(
        [weakSelf](const TcpConnectionPtr &c) {
            if (auto self = weakSelf.lock())
            {
                self->removeConnection(c);
            }
            // Else the TcpClient instance has already been destroyed
            else
            {
                LOG_TRACE << "TcpClient::removeConnection was skipped because "
                             "TcpClient instanced already freed";
                c->getLoop()->queueInLoop([c] { c->connectDestroyed(); });
            }
        });
    conn->setCloseCallback(std::move(closeCb));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->setSSLErrorCallback([this](SSLError err) {
        if (sslErrorCallback_)
        {
            sslErrorCallback_(err);
        }
    });

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

    loop_->queueInLoop([conn]() { conn->connectDestroyed(); });
    if (retry_ && connect_)
    {
        LOG_TRACE << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                  << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}

void TcpClient::enableSSL(
    bool useOldTLS,
    bool validateCert,
    std::string hostname,
    const std::vector<std::pair<std::string, std::string>> &sslConfCmds,
    const std::string &certPath,
    const std::string &keyPath,
    const std::string &caPath)
{
    if (!hostname.empty())
    {
        std::transform(hostname.begin(),
                       hostname.end(),
                       hostname.begin(),
                       [](unsigned char c) { return tolower(c); });
    }

    sslPolicyPtr_ = std::make_shared<SSLPolicy>();
    sslPolicyPtr_->setValidate(validateCert)
        .setUseOldTLS(useOldTLS)
        .setConfCmds(sslConfCmds)
        .setCertPath(certPath)
        .setKeyPath(keyPath)
        .setHostname(hostname)
        .setValidate(validateCert)
        .setCaPath(caPath);
    sslContextPtr_ = newSSLContext(*sslPolicyPtr_, false);
}

void TcpClient::startEncryption(const TcpConnectionPtr &conn,
                                SSLPolicyPtr policy)
{
    if (conn->isSSLConnection())
    {
        LOG_WARN << "The connection is already encrypted";
        return;
    }
    loop_->runInLoop(
        [this, conn, policy]() { startEncryptionInLoop(conn, policy); });
}

void TcpClient::startEncryptionInLoop(const TcpConnectionPtr &conn,
                                      SSLPolicyPtr policy)
{
    auto sslContextPtr = newSSLContext(*policy, false);
    auto recvCallback = conn->recvMsgCallback_;
    auto writeCompleteCallback = conn->writeCompleteCallback_;
    auto closeCallback = conn->closeCallback_;
    auto highWaterMarkCallback = conn->highWaterMarkCallback_;
    auto sslErrorCallback = conn->sslErrorCallback_;
    auto sslConn = newTLSConnection(conn, policy, sslContextPtr);

    if (!policy->getOneShotConnctionCallback())
        policy->setOneShotConnctionCallback([]() {});
    if (recvCallback)
        sslConn->setRecvMsgCallback(std::move(recvCallback));
    if (writeCompleteCallback)
        sslConn->setWriteCompleteCallback(std::move(writeCompleteCallback));
    if (closeCallback)
        sslConn->setCloseCallback(std::move(closeCallback));
    if (sslErrorCallback)
        sslConn->setSSLErrorCallback(std::move(sslErrorCallback)
    if (highWaterMarkCallback)
        sslConn->highWaterMarkCallback_ = std::move(highWaterMarkCallback);

    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = sslConn;
    sslConn->startHandshake(*conn->getRecvBuffer());
}