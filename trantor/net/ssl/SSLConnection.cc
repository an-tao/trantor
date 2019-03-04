/**
 *
 *  SSLConnection.h
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

#include <trantor/net/ssl/SSLConnection.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/inner/Channel.h>
#include <trantor/net/inner/Socket.h>
#include <openssl/err.h>
using namespace trantor;
SSLConnection::SSLConnection(EventLoop *loop, int socketfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr,
                             const std::shared_ptr<SSL_CTX> &ctxPtr,
                             bool isServer)
    : TcpConnectionImpl(loop, socketfd, localAddr, peerAddr),
      _sslPtr(std::shared_ptr<SSL>(SSL_new(ctxPtr.get()), [](SSL *ssl) { SSL_free(ssl); })),
      _isServer(isServer)
{
    assert(_sslPtr);
    auto r = SSL_set_fd(_sslPtr.get(), socketfd);
    (void)r;
    assert(r);
    _isSSLConn = true;
}

void SSLConnection::writeCallback()
{
    LOG_TRACE << "write Callback";
    _loop->assertInLoopThread();
    if (_status == SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if (_status == SSLStatus::Connected)
    {
        TcpConnectionImpl::writeCallback();
    }
}

void SSLConnection::readCallback()
{
    LOG_TRACE << "read Callback";
    _loop->assertInLoopThread();
    if (_status == SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if (_status == SSLStatus::Connected)
    {
        int rd;
        char buf[65536];
        bool newDataFlag = false;
        do
        {

            rd = SSL_read(_sslPtr.get(), buf, sizeof(buf));
            LOG_TRACE << "ssl read:" << rd << " bytes";
            int sslerr = SSL_get_error(_sslPtr.get(), rd);
            if (rd <= 0 && sslerr != SSL_ERROR_WANT_READ)
            {
                LOG_TRACE << "ssl read err:" << sslerr;
                _status = SSLStatus::DisConnected;
                handleClose();
                return;
            }
            if (rd > 0)
            {
                _readBuffer.append(buf, rd);
                newDataFlag = true;
            }

        } while (rd == sizeof(buf));
        if (newDataFlag)
        {
            //eval callback function;
            _recvMsgCallback(shared_from_this(), &_readBuffer);
        }
    }
}

void SSLConnection::connectEstablished()
{
    _loop->runInLoop([=]() {
        LOG_TRACE << "connectEstablished";
        assert(_state == Connecting);
        _ioChannelPtr->tie(shared_from_this());
        _ioChannelPtr->enableReading();
        _state = Connected;
        if (_isServer)
        {
            SSL_set_accept_state(_sslPtr.get());
        }
        else
        {
            _ioChannelPtr->enableWriting();
            SSL_set_connect_state(_sslPtr.get());
        }
    });
}
void SSLConnection::doHandshaking()
{
    assert(_status == SSLStatus::Handshaking);
    int r = SSL_do_handshake(_sslPtr.get());
    if (r == 1)
    {
        _status = SSLStatus::Connected;
        _connectionCallback(shared_from_this());
        return;
    }
    int err = SSL_get_error(_sslPtr.get(), r);
    if (err == SSL_ERROR_WANT_WRITE)
    { //SSL want writable;
        _ioChannelPtr->enableWriting();
        _ioChannelPtr->disableReading();
    }
    else if (err == SSL_ERROR_WANT_READ)
    { //SSL want readable;
        _ioChannelPtr->enableReading();
        _ioChannelPtr->disableWriting();
    }
    else
    { //错误
        //ERR_print_errors(err);
        LOG_FATAL << "SSL handshake err";
        _ioChannelPtr->disableReading();
        _status = SSLStatus::DisConnected;
        forceClose();
    }
}

ssize_t SSLConnection::writeInLoop(const char *buffer, size_t length)
{
    LOG_TRACE << "send in loop";
    _loop->assertInLoopThread();
    if (_state != Connected)
    {
        LOG_WARN << "Connection is not connected,give up sending";
        return -1;
    }
    if (_status != SSLStatus::Connected)
    {
        LOG_WARN << "SSL is not connected,give up sending";
        return -1;
    }

    //send directly
    auto sendLen = SSL_write(_sslPtr.get(), buffer, length);
    int sslerr = SSL_get_error(_sslPtr.get(), sendLen);
    if (sendLen < 0 && sslerr != SSL_ERROR_WANT_WRITE)
    {
        LOG_ERROR << "ssl write error:" << sslerr;
        return -1;
    }
    return sendLen;
}
