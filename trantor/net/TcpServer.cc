/**
 *
 *  TcpServer.cc
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

#include <trantor/net/TcpServer.h>
#include "Acceptor.h"
#include "inner/TcpConnectionImpl.h"
#include <trantor/utils/Logger.h>
#include <functional>
#include <vector>
#ifdef USE_OPENSSL
#include "ssl/SSLConnection.h"
#endif
using namespace trantor;
using namespace std::placeholders;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &address,
                     const std::string &name)
    : loop_(loop),
      acceptorPtr_(new Acceptor(loop, address)),
      serverName_(name),
      recvMessageCallback_([](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
          LOG_ERROR << "unhandled recv message [" << buffer->readableBytes() << " bytes]";
          buffer->retrieveAll();
      })
{
    acceptorPtr_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << serverName_ << "] destructing";
}

void TcpServer::newConnection(int sockfd, const InetAddress &peer)
{
    LOG_TRACE << "new connection:fd=" << sockfd << " address=" << peer.toIpPort();
    //test code for blocking or nonblocking
    //    std::vector<char> str(1024*1024*100);
    //    for(int i=0;i<str.size();i++)
    //        str[i]='A';
    //    LOG_TRACE<<"vector size:"<<str.size();
    //    size_t n=write(sockfd,&str[0],str.size());
    //    LOG_TRACE<<"write "<<n<<" bytes";
    loop_->assertInLoopThread();
    EventLoop *ioLoop = NULL;
    if (loopPoolPtr_ && loopPoolPtr_->getLoopNum() > 0)
    {
        ioLoop = loopPoolPtr_->getNextLoop();
    }
    if (ioLoop == NULL)
        ioLoop = loop_;
#ifdef USE_OPENSSL
    std::shared_ptr<TcpConnectionImpl> newPtr;
    if (_sslCtxPtr)
    {
        newPtr = std::make_shared<SSLConnection>(ioLoop,
                                                 sockfd,
                                                 InetAddress(Socket::getLocalAddr(sockfd)),
                                                 peer,
                                                 _sslCtxPtr);
    }
    else
    {
        newPtr = std::make_shared<TcpConnectionImpl>(ioLoop,
                                                     sockfd,
                                                     InetAddress(Socket::getLocalAddr(sockfd)),
                                                     peer);
    }
#else
    auto newPtr = std::make_shared<TcpConnectionImpl>(ioLoop, sockfd, InetAddress(Socket::getLocalAddr(sockfd)), peer);
#endif

    if (_idleTimeout > 0)
    {
        assert(_timeingWheelMap[ioLoop]);
        newPtr->enableKickingOff(_idleTimeout, _timeingWheelMap[ioLoop]);
    }
    newPtr->setRecvMsgCallback(recvMessageCallback_);

    newPtr->setConnectionCallback([=](const TcpConnectionPtr &connectionPtr) {
        if (connectionCallback_)
            connectionCallback_(connectionPtr);
    });
    newPtr->setWriteCompleteCallback([=](const TcpConnectionPtr &connectionPtr) {
        if (writeCompleteCallback_)
            writeCompleteCallback_(connectionPtr);
    });
    newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed, this, _1));
    connSet_.insert(newPtr);
    newPtr->connectEstablished();
}

void TcpServer::start()
{
    loop_->runInLoop([=]() {
        assert(!_started);
        _started = true;
        if (_idleTimeout > 0)
        {
            _timeingWheelMap[loop_] = std::make_shared<TimingWheel>(loop_,
                                                                    _idleTimeout,
                                                                    1,
                                                                    _idleTimeout < 500 ? _idleTimeout + 1 : 100);
            if (loopPoolPtr_)
            {
                auto loopNum = loopPoolPtr_->getLoopNum();
                while (loopNum > 0)
                {
                    //LOG_TRACE << "new Wheel loopNum=" << loopNum;
                    auto poolLoop = loopPoolPtr_->getNextLoop();
                    _timeingWheelMap[poolLoop] = std::make_shared<TimingWheel>(poolLoop,
                                                                               _idleTimeout,
                                                                               1,
                                                                               _idleTimeout < 500 ? _idleTimeout + 1 : 100);
                    loopNum--;
                }
            }
        }
        LOG_TRACE << "map size=" << _timeingWheelMap.size();
        acceptorPtr_->listen();
    });
}

void TcpServer::setIoLoopNum(size_t num)
{
    assert(num >= 0);
    loop_->runInLoop([=]() {
        assert(!_started);
        loopPoolPtr_ = std::unique_ptr<EventLoopThreadPool>(new EventLoopThreadPool(num));
        loopPoolPtr_->start();
    });
}

void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
{
    LOG_TRACE << "connectionClosed";
    //loop_->assertInLoopThread();
    loop_->runInLoop([=]() {
        size_t n = connSet_.erase(connectionPtr);
        (void)n;
        assert(n == 1);
    });

    std::dynamic_pointer_cast<TcpConnectionImpl>(connectionPtr)->connectDestroyed();
}

const std::string TcpServer::ipPort() const
{
    return acceptorPtr_->addr().toIpPort();
}

#ifdef USE_OPENSSL
void TcpServer::enableSSL(const std::string &certPath, const std::string &keyPath)
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

    auto r = SSL_CTX_use_certificate_file(_sslCtxPtr.get(), certPath.c_str(), SSL_FILETYPE_PEM);
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
    r = SSL_CTX_use_PrivateKey_file(_sslCtxPtr.get(), keyPath.c_str(), SSL_FILETYPE_PEM);
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
    r = SSL_CTX_check_private_key(_sslCtxPtr.get());
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
}
#endif
