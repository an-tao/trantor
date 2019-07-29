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
#include "ssl/SSLConnection.h"
#include "Connector.h"
#include "inner/TcpConnectionImpl.h"
#include <trantor/net/EventLoop.h>
#include "Socket.h"

#include <stdio.h>  // snprintf
#include <functional>

using namespace trantor;
using namespace std::placeholders;

// TcpClient::TcpClient(EventLoop* loop)
//   : _loop(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : _loop(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace trantor
{
void removeConnector(const ConnectorPtr &connector)
{
    // connector->
}

TcpClient::IgnoreSigPipe TcpClient::initObj;

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
    : _loop(loop),
      _connector(new Connector(loop, serverAddr, false)),
      _name(nameArg),
      _connectionCallback(defaultConnectionCallback),
      _messageCallback(defaultMessageCallback),
      _retry(false),
      _connect(true)
{
    _connector->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    _connector->setErrorCallback([=]() {
        if (_connectionErrorCallback)
        {
            _connectionErrorCallback();
        }
    });
    LOG_TRACE << "TcpClient::TcpClient[" << _name << "] - connector ";
}

TcpClient::~TcpClient()
{
    LOG_TRACE << "TcpClient::~TcpClient[" << _name << "] - connector ";
    TcpConnectionImplPtr conn;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        conn = std::dynamic_pointer_cast<TcpConnectionImpl>(_connection);
    }
    if (conn)
    {
        assert(_loop == conn->getLoop());
        // TODO: not 100% safe, if we are in different thread
        auto loop = _loop;
        _loop->runInLoop([conn, loop]() {
            conn->setCloseCallback([loop](const TcpConnectionPtr &connPtr) {
                loop->queueInLoop([connPtr]() {
                    static_cast<TcpConnectionImpl *>(connPtr.get())
                        ->connectDestroyed();
                });
            });
        });
        conn->forceClose();
    }
    else
    {
        /// TODO need test in this condition
        _connector->stop();
        _loop->runAfter(1, [=]() { trantor::removeConnector(_connector); });
    }
}

void TcpClient::connect()
{
    // TODO: check state
    LOG_TRACE << "TcpClient::connect[" << _name << "] - connecting to "
              << _connector->serverAddress().toIpPort();
    _connect = true;
    _connector->start();
}

void TcpClient::disconnect()
{
    _connect = false;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_connection)
        {
            _connection->shutdown();
        }
    }
}

void TcpClient::stop()
{
    _connect = false;
    _connector->stop();
}

void TcpClient::newConnection(int sockfd)
{
    _loop->assertInLoopThread();
    InetAddress peerAddr(Socket::getPeerAddr(sockfd));
    InetAddress localAddr(Socket::getLocalAddr(sockfd));
    // TODO poll with zero timeout to double confirm the new connection
    // TODO use make_shared if necessary
    std::shared_ptr<TcpConnectionImpl> conn;
    if (_sslCtxPtr)
    {
        conn = std::make_shared<SSLConnection>(
            _loop, sockfd, localAddr, peerAddr, _sslCtxPtr, false);
    }
    else
    {
        conn = std::make_shared<TcpConnectionImpl>(_loop,
                                                   sockfd,
                                                   localAddr,
                                                   peerAddr);
    }
    conn->setConnectionCallback(_connectionCallback);
    conn->setRecvMsgCallback(_messageCallback);
    conn->setWriteCompleteCallback(_writeCompleteCallback);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _connection = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    _loop->assertInLoopThread();
    assert(_loop == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(_mutex);
        assert(_connection == conn);
        _connection.reset();
    }

    _loop->queueInLoop(
        std::bind(&TcpConnectionImpl::connectDestroyed,
                  std::dynamic_pointer_cast<TcpConnectionImpl>(conn)));
    if (_retry && _connect)
    {
        LOG_TRACE << "TcpClient::connect[" << _name << "] - Reconnecting to "
                  << _connector->serverAddress().toIpPort();
        _connector->restart();
    }
}

void TcpClient::enableSSL()
{
    // init OpenSSL
    initOpenSSL();
    /* Create a new OpenSSL context */
    _sslCtxPtr = newSSLContext();
}
