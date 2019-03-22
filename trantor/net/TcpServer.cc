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
    : _loop(loop),
      _acceptorPtr(new Acceptor(loop, address)),
      _serverName(name),
      _recvMessageCallback([](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
          LOG_ERROR << "unhandled recv message [" << buffer->readableBytes() << " bytes]";
          buffer->retrieveAll();
      })
{
    _acceptorPtr->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    _loop->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << _serverName << "] destructing";
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
    _loop->assertInLoopThread();
    EventLoop *ioLoop = NULL;
    if (_loopPoolPtr && _loopPoolPtr->getLoopNum() > 0)
    {
        ioLoop = _loopPoolPtr->getNextLoop();
    }
    if (ioLoop == NULL)
        ioLoop = _loop;
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
        assert(_timingWheelMap[ioLoop]);
        newPtr->enableKickingOff(_idleTimeout, _timingWheelMap[ioLoop]);
    }
    newPtr->setRecvMsgCallback(_recvMessageCallback);

    newPtr->setConnectionCallback([=](const TcpConnectionPtr &connectionPtr) {
        if (_connectionCallback)
            _connectionCallback(connectionPtr);
    });
    newPtr->setWriteCompleteCallback([=](const TcpConnectionPtr &connectionPtr) {
        if (_writeCompleteCallback)
            _writeCompleteCallback(connectionPtr);
    });
    newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed, this, _1));
    _connSet.insert(newPtr);
    newPtr->connectEstablished();
}

void TcpServer::start()
{
    _loop->runInLoop([=]() {
        assert(!_started);
        _started = true;
        if (_idleTimeout > 0)
        {
            _timingWheelMap[_loop] = std::make_shared<TimingWheel>(_loop,
                                                                   _idleTimeout,
                                                                   1,
                                                                   _idleTimeout < 500 ? _idleTimeout + 1 : 100);
            if (_loopPoolPtr)
            {
                auto loopNum = _loopPoolPtr->getLoopNum();
                while (loopNum > 0)
                {
                    //LOG_TRACE << "new Wheel loopNum=" << loopNum;
                    auto poolLoop = _loopPoolPtr->getNextLoop();
                    _timingWheelMap[poolLoop] = std::make_shared<TimingWheel>(poolLoop,
                                                                              _idleTimeout,
                                                                              1,
                                                                              _idleTimeout < 500 ? _idleTimeout + 1 : 100);
                    loopNum--;
                }
            }
        }
        LOG_TRACE << "map size=" << _timingWheelMap.size();
        _acceptorPtr->listen();
    });
}

void TcpServer::setIoLoopNum(size_t num)
{
    assert(num >= 0);
    assert(!_started);
    _loopPoolPtr = std::unique_ptr<EventLoopThreadPool>(new EventLoopThreadPool(num));
    _loopPoolPtr->start();
}

void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
{
    LOG_TRACE << "connectionClosed";
    //_loop->assertInLoopThread();
    _loop->runInLoop([=]() {
        size_t n = _connSet.erase(connectionPtr);
        (void)n;
        assert(n == 1);
    });

    std::dynamic_pointer_cast<TcpConnectionImpl>(connectionPtr)->connectDestroyed();
}

const std::string TcpServer::ipPort() const
{
    return _acceptorPtr->addr().toIpPort();
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
