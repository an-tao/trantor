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

#include "Acceptor.h"
#include "inner/TcpConnectionImpl.h"
#include "ssl/SSLConnection.h"
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
    : _loop(loop),
      _acceptorPtr(new Acceptor(loop, address, reUseAddr, reUsePort)),
      _serverName(name),
      _recvMessageCallback(
          [](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
              LOG_ERROR << "unhandled recv message [" << buffer->readableBytes()
                        << " bytes]";
              buffer->retrieveAll();
          })
{
    _acceptorPtr->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    _loop->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << _serverName << "] destructing";
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
    _loop->assertInLoopThread();
    EventLoop *ioLoop = NULL;
    if (_loopPoolPtr && _loopPoolPtr->getLoopNum() > 0)
    {
        ioLoop = _loopPoolPtr->getNextLoop();
    }
    if (ioLoop == NULL)
        ioLoop = _loop;
    std::shared_ptr<TcpConnectionImpl> newPtr;
    if (_sslCtxPtr)
    {
        newPtr =
            std::make_shared<SSLConnection>(ioLoop,
                                            sockfd,
                                            InetAddress(
                                                Socket::getLocalAddr(sockfd)),
                                            peer,
                                            _sslCtxPtr);
    }
    else
    {
        newPtr = std::make_shared<TcpConnectionImpl>(
            ioLoop, sockfd, InetAddress(Socket::getLocalAddr(sockfd)), peer);
    }

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
    newPtr->setWriteCompleteCallback(
        [=](const TcpConnectionPtr &connectionPtr) {
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
            _timingWheelMap[_loop] =
                std::make_shared<TimingWheel>(_loop,
                                              _idleTimeout,
                                              1,
                                              _idleTimeout < 500
                                                  ? _idleTimeout + 1
                                                  : 100);
            if (_loopPoolPtr)
            {
                auto loopNum = _loopPoolPtr->getLoopNum();
                while (loopNum > 0)
                {
                    // LOG_TRACE << "new Wheel loopNum=" << loopNum;
                    auto poolLoop = _loopPoolPtr->getNextLoop();
                    _timingWheelMap[poolLoop] =
                        std::make_shared<TimingWheel>(poolLoop,
                                                      _idleTimeout,
                                                      1,
                                                      _idleTimeout < 500
                                                          ? _idleTimeout + 1
                                                          : 100);
                    loopNum--;
                }
            }
        }
        LOG_TRACE << "map size=" << _timingWheelMap.size();
        _acceptorPtr->listen();
    });
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

    static_cast<TcpConnectionImpl *>(connectionPtr.get())->connectDestroyed();
}

const std::string TcpServer::ipPort() const
{
    return _acceptorPtr->addr().toIpPort();
}

void TcpServer::enableSSL(const std::string &certPath,
                          const std::string &keyPath)
{
    // init OpenSSL
    initOpenSSL();
    /* Create a new OpenSSL context */
    _sslCtxPtr = newSSLContext();
    initServerSSLContext(_sslCtxPtr, certPath, keyPath);
}
