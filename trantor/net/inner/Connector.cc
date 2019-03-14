/**
 *
 *  Connector.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 * 
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#include "Connector.h"
#include "Channel.h"
#include "Socket.h"
using namespace trantor;

Connector::Connector(EventLoop *loop, const InetAddress &addr, bool retry)
    : _loop(loop),
      _serverAddr(addr),
      _connect(false),
      _state(kDisconnected),
      _retryInterval(kInitRetryDelayMs),
      _maxRetryInterval(kMaxRetryDelayMs),
      _retry(retry)
{
}
Connector::Connector(EventLoop *loop, InetAddress &&addr, bool retry)
    : _loop(loop),
      _serverAddr(std::move(addr)),
      _connect(false),
      _state(kDisconnected),
      _retryInterval(kInitRetryDelayMs),
      _maxRetryInterval(kMaxRetryDelayMs),
      _retry(retry)
{
}

void Connector::start()
{
    _connect = true;
    _loop->runInLoop([=]() {
        startInLoop();
    });
}
void Connector::restart()
{
}
void Connector::stop()
{
}

void Connector::startInLoop()
{
    _loop->assertInLoopThread();
    assert(_state == kDisconnected);
    if (_connect)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}
void Connector::connect()
{
    int sockfd = Socket::createNonblockingSocketOrDie(_serverAddr.family());
    int ret = Socket::connect(sockfd, _serverAddr);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        LOG_TRACE << "connecting";
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        if (_errorCallback)
            _errorCallback();
        break;

    default:
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        if (_errorCallback)
            _errorCallback();
        break;
    }
}

void Connector::connecting(int sockfd)
{
    _state = kConnecting;
    assert(!_channelPtr);
    _channelPtr.reset(new Channel(_loop, sockfd));
    _channelPtr->setWriteCallback(
        std::bind(&Connector::handleWrite, shared_from_this()));
    _channelPtr->setErrorCallback(
        std::bind(&Connector::handleError, shared_from_this()));
    _channelPtr->setCloseCallback(
        std::bind(&Connector::handleError, shared_from_this()));
    LOG_TRACE << "connecting:" << sockfd;
    _channelPtr->enableWriting();
}

int Connector::removeAndResetChannel()
{
    _channelPtr->disableAll();
    _channelPtr->remove();
    int sockfd = _channelPtr->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    _loop->queueInLoop([=]() {
        _channelPtr.reset();
    });
    return sockfd;
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << _state;

    if (_state == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            if (_retry)
            {
                retry(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
            if (_errorCallback)
            {
                _errorCallback();
            }
        }
        else if (Socket::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            if (_retry)
            {
                retry(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
            if (_errorCallback)
            {
                _errorCallback();
            }
        }
        else
        {
            _state = kConnected;
            if (_connect)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(_state == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << _state;
    if (_state == kConnecting)
    {
        _state = kDisconnected;
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        if (_retry)
        {
            retry(sockfd);
        }
        else
        {
            ::close(sockfd);
        }
        if (_errorCallback)
        {
            _errorCallback();
        }
    }
}

void Connector::retry(int sockfd)
{
    assert(_retry);
    ::close(sockfd);
    _state = kDisconnected;
    if (_connect)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << _serverAddr.toIpPort()
                 << " in " << _retryInterval << " milliseconds. ";
        _loop->runAfter(_retryInterval / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        _retryInterval = std::min(_retryInterval * 2, _maxRetryInterval);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}
