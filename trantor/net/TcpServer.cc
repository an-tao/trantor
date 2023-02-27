/**
 *
 *  @file TcpServer.cc
 *  @author An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/trantor
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *  Trantor
 *
 */

#include "Acceptor.h"
#include "inner/TcpConnectionImpl.h"
#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <functional>
#include <vector>
using namespace trantor;
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &address,
                     const std::string &name,
                     bool reUseAddr,
                     bool reUsePort)
    : loop_(loop),
      acceptorPtr_(new Acceptor(loop, address, reUseAddr, reUsePort)),
      serverName_(name),
      recvMessageCallback_([](const TcpConnectionPtr &, MsgBuffer *buffer) {
          LOG_ERROR << "unhandled recv message [" << buffer->readableBytes()
                    << " bytes]";
          buffer->retrieveAll();
      })
{
    acceptorPtr_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    // loop_->assertInLoopThread();
    if (tlsInitiatorPtr_)
        tlsInitiatorPtr_->serverDistructed = true;
    LOG_TRACE << "TcpServer::~TcpServer [" << serverName_ << "] destructing";
}

void TcpServer::newConnection(int sockfd, const InetAddress &peer)
{
    LOG_TRACE << "new connection:fd=" << sockfd
              << " address=" << peer.toIpPort();
    // test code for blocking or nonblocking
    //    std::vector<char> str(1024*1024*100);
    //    for(int i=0;i<str.size();i++)
    //        str[i]='A';
    //    LOG_TRACE<<"vector size:"<<str.size();
    //    size_t n=write(sockfd,&str[0],str.size());
    //    LOG_TRACE<<"write "<<n<<" bytes";
    loop_->assertInLoopThread();
    EventLoop *ioLoop = NULL;
    if (loopPoolPtr_ && loopPoolPtr_->size() > 0)
    {
        ioLoop = loopPoolPtr_->getNextLoop();
    }
    if (ioLoop == NULL)
        ioLoop = loop_;
    if (tlsInitiatorPtr_ == nullptr)
        tlsInitiatorPtr_ = std::make_shared<ServerTlsInitiator>(this);
    TcpConnectionPtr newPtr;
    if (policyPtr_)
    {
        assert(sslContextPtr_);
        newPtr = std::make_shared<TcpConnectionImpl>(
            ioLoop,
            sockfd,
            InetAddress(Socket::getLocalAddr(sockfd)),
            peer,
            tlsInitiatorPtr_,
            policyPtr_,
            sslContextPtr_);
    }
    else
    {
        newPtr = std::make_shared<TcpConnectionImpl>(
            ioLoop,
            sockfd,
            InetAddress(Socket::getLocalAddr(sockfd)),
            peer,
            tlsInitiatorPtr_);
    }

    if (idleTimeout_ > 0)
    {
        assert(timingWheelMap_[ioLoop]);
        newPtr->enableKickingOff(idleTimeout_, timingWheelMap_[ioLoop]);
    }
    newPtr->setRecvMsgCallback(recvMessageCallback_);

    newPtr->setConnectionCallback(
        [this](const TcpConnectionPtr &connectionPtr) {
            if (connectionCallback_)
                connectionCallback_(connectionPtr);
        });
    newPtr->setWriteCompleteCallback(
        [this](const TcpConnectionPtr &connectionPtr) {
            if (writeCompleteCallback_)
                writeCompleteCallback_(connectionPtr);
        });
    newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed, this, _1));
    connSet_.insert(newPtr);
    newPtr->connectEstablished();
}

void TcpServer::start()
{
    loop_->runInLoop([this]() {
        assert(!started_);
        started_ = true;
        if (idleTimeout_ > 0)
        {
            timingWheelMap_[loop_] =
                std::make_shared<TimingWheel>(loop_,
                                              idleTimeout_,
                                              1.0F,
                                              idleTimeout_ < 500
                                                  ? idleTimeout_ + 1
                                                  : 100);
            if (loopPoolPtr_)
            {
                auto loopNum = loopPoolPtr_->size();
                while (loopNum > 0)
                {
                    // LOG_TRACE << "new Wheel loopNum=" << loopNum;
                    auto poolLoop = loopPoolPtr_->getNextLoop();
                    timingWheelMap_[poolLoop] =
                        std::make_shared<TimingWheel>(poolLoop,
                                                      idleTimeout_,
                                                      1.0F,
                                                      idleTimeout_ < 500
                                                          ? idleTimeout_ + 1
                                                          : 100);
                    --loopNum;
                }
            }
        }
        LOG_TRACE << "map size=" << timingWheelMap_.size();
        acceptorPtr_->listen();
    });
}
void TcpServer::stop()
{
    if (loop_->isInLoopThread())
    {
        acceptorPtr_.reset();
        // copy the connSet_ to a vector, use the vector to close the
        // connections to avoid the iterator invalidation.
        std::vector<TcpConnectionPtr> connPtrs;
        connPtrs.reserve(connSet_.size());
        for (auto &conn : connSet_)
        {
            connPtrs.push_back(conn);
        }
        for (auto connection : connPtrs)
        {
            connection->forceClose();
        }
    }
    else
    {
        std::promise<void> pro;
        auto f = pro.get_future();
        loop_->queueInLoop([this, &pro]() {
            acceptorPtr_.reset();
            std::vector<TcpConnectionPtr> connPtrs;
            connPtrs.reserve(connSet_.size());
            for (auto &conn : connSet_)
            {
                connPtrs.push_back(conn);
            }
            for (auto connection : connPtrs)
            {
                connection->forceClose();
            }
            pro.set_value();
        });
        f.get();
    }
    loopPoolPtr_.reset();
    for (auto &iter : timingWheelMap_)
    {
        std::promise<void> pro;
        auto f = pro.get_future();
        iter.second->getLoop()->runInLoop([&iter, &pro]() mutable {
            iter.second.reset();
            pro.set_value();
        });
        f.get();
    }
}
void TcpServer::handleCloseInLoop(const TcpConnectionPtr &connectionPtr)
{
    size_t n = connSet_.erase(connectionPtr);
    (void)n;
    assert(n == 1);
    auto connLoop = connectionPtr->getLoop();

    // NOTE: always queue this operation in connLoop, because this connection
    // may be in loop_'s current active channels, waiting to be processed.
    // If `connectDestroyed()` is called here, we will be using an wild pointer
    // later.
    connLoop->queueInLoop(
        [connectionPtr]() { connectionPtr->connectDestroyed(); });
}
void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
{
    LOG_TRACE << "connectionClosed";
    if (loop_->isInLoopThread())
    {
        handleCloseInLoop(connectionPtr);
    }
    else
    {
        loop_->queueInLoop(
            [this, connectionPtr]() { handleCloseInLoop(connectionPtr); });
    }
}

const std::string TcpServer::ipPort() const
{
    return acceptorPtr_->addr().toIpPort();
}

const trantor::InetAddress &TcpServer::address() const
{
    return acceptorPtr_->addr();
}

void TcpServer::enableSSL(
    const std::string &certPath,
    const std::string &keyPath,
    bool useOldTLS,
    const std::vector<std::pair<std::string, std::string>> &sslConfCmds,
    const std::string &caPath)
{
    policyPtr_ = std::make_shared<SSLPolicy>();
    policyPtr_->setKeyPath(keyPath)
        .setCertPath(certPath)
        .setUseOldTLS(useOldTLS)
        .setConfCmds(sslConfCmds)
        .setCaPath(caPath)
        .setValidate(caPath.empty() ? false : true);
    sslContextPtr_ = newSSLContext(*policyPtr_, true);
}

void TcpServer::ServerTlsInitiator::startEncryption(
    const TcpConnectionPtr &conn,
    SSLPolicyPtr policy)
{
    abort();
    // if (serverDistructed)
    // {
    //     LOG_WARN << "TcpServer have destructed. Cannot start TLS on "
    //                 "connections managed by that server.";
    //     return;
    // }

    // if (conn->isSSLConnection())
    // {
    //     LOG_WARN
    //         << "Connection already started TLS. Cannot start TLS on it again.";
    //     return;
    // }

    // auto it = server_->connSet_.find(conn);
    // if (it == server_->connSet_.end())
    // {
    //     LOG_WARN
    //         << "Connection not managed by this server. Cannot start TLS on it.";
    //     return;
    // }

    // auto rawConn = *it;
    // server_->connSet_.erase(it);

    // SSLPolicy *pol = nullptr;
    // if (policy)
    //     pol = policy.get();
    // else if (server_->policyPtr_)
    //     pol = server_->policyPtr_.get();
    // else
    // {
    //     LOG_FATAL << "No SSLPolicy found. (Likely caused by API misuse.)";
    //     return;
    // }
    // assert(pol != nullptr);

    // if (!pol->getOneShotConnctionCallback())
    //     pol->setOneShotConnctionCallback([](const TcpConnectionPtr &) {});
    // server_->sslContextPtr_ = newSSLContext(*pol, true);
    // auto recvCallback = rawConn->recvMsgCallback_;
    // auto connCallback = rawConn->connectionCallback_;
    // auto closeCallback = rawConn->closeCallback_;
    // auto highWaterMarkCallback = rawConn->highWaterMarkCallback_;
    // auto sslErrorCallback = rawConn->sslErrorCallback_;
    // auto tlsConn = newTLSConnection(rawConn, policy, server_->sslContextPtr_);
    // if (recvCallback)
    //     tlsConn->setRecvMsgCallback(std::move(recvCallback));
    // if (connCallback)
    //     tlsConn->setConnectionCallback(std::move(connCallback));
    // if (closeCallback)
    //     tlsConn->setCloseCallback(std::move(closeCallback));
    // if (sslErrorCallback)
    //     tlsConn->setSSLErrorCallback(std::move(sslErrorCallback));
    // if (highWaterMarkCallback)
    //     tlsConn->highWaterMarkCallback_ = std::move(highWaterMarkCallback);

    // server_->connSet_.insert(tlsConn);
    // tlsConn->startHandshake(*rawConn->getRecvBuffer());
}