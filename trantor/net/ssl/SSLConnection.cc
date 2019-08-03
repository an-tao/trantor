/**
 *
 *  SSLConnection.cc
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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>

using namespace trantor;

namespace trantor
{
void initOpenSSL()
{
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || \
    (defined(LIBRESSL_VERSION_NUMBER) &&      \
     LIBRESSL_VERSION_NUMBER < 0x20700000L)
    // Initialize OpenSSL once;
    static std::once_flag once;
    std::call_once(once, []() {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    });
#endif
}
class SSLContext
{
  public:
    SSLContext()
    {
        _ctxPtr = SSL_CTX_new(SSLv23_method());
    }
    ~SSLContext()
    {
        if (_ctxPtr)
        {
            SSL_CTX_free(_ctxPtr);
        }
    }

    SSL_CTX *get()
    {
        return _ctxPtr;
    }

  private:
    SSL_CTX *_ctxPtr;
};
class SSLConn
{
  public:
    SSLConn(SSL_CTX *ctx)
    {
        _SSL = SSL_new(ctx);
    }
    ~SSLConn()
    {
        if (_SSL)
        {
            SSL_free(_SSL);
        }
    }
    SSL *get()
    {
        return _SSL;
    }

  private:
    SSL *_SSL;
};

std::shared_ptr<SSLContext> newSSLContext()
{
    return std::make_shared<SSLContext>();
}
void initServerSSLContext(const std::shared_ptr<SSLContext> &ctx,
                          const std::string &certPath,
                          const std::string &keyPath)
{
    auto r = SSL_CTX_use_certificate_file(ctx->get(),
                                          certPath.c_str(),
                                          SSL_FILETYPE_PEM);
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
    r = SSL_CTX_use_PrivateKey_file(ctx->get(),
                                    keyPath.c_str(),
                                    SSL_FILETYPE_PEM);
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
    r = SSL_CTX_check_private_key(ctx->get());
    if (!r)
    {
        LOG_FATAL << strerror(errno);
        abort();
    }
}
}  // namespace trantor

SSLConnection::SSLConnection(EventLoop *loop,
                             int socketfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr,
                             const std::shared_ptr<SSLContext> &ctxPtr,
                             bool isServer)
    : TcpConnectionImpl(loop, socketfd, localAddr, peerAddr),
      _sslPtr(std::make_shared<SSLConn>(ctxPtr->get())),
      _isServer(isServer)
{
    assert(_sslPtr);
    auto r = SSL_set_fd(_sslPtr->get(), socketfd);
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
        bool newDataFlag = false;
        size_t readLength;
        do
        {
            _readBuffer.ensureWritableBytes(1024);
            readLength = _readBuffer.writableBytes();
            rd = SSL_read(_sslPtr->get(), _readBuffer.beginWrite(), readLength);
            LOG_TRACE << "ssl read:" << rd << " bytes";
            if (rd <= 0)
            {
                int sslerr = SSL_get_error(_sslPtr->get(), rd);
                if (sslerr == SSL_ERROR_WANT_READ)
                {
                    break;
                }
                else
                {
                    LOG_TRACE << "ssl read err:" << sslerr;
                    _status = SSLStatus::DisConnected;
                    handleClose();
                    return;
                }
            }
            _readBuffer.hasWritten(rd);
            newDataFlag = true;
        } while ((size_t)rd == readLength);
        if (newDataFlag)
        {
            // Run callback function
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
            SSL_set_accept_state(_sslPtr->get());
        }
        else
        {
            _ioChannelPtr->enableWriting();
            SSL_set_connect_state(_sslPtr->get());
        }
    });
}
void SSLConnection::doHandshaking()
{
    assert(_status == SSLStatus::Handshaking);
    int r = SSL_do_handshake(_sslPtr->get());
    if (r == 1)
    {
        _status = SSLStatus::Connected;
        _connectionCallback(shared_from_this());
        return;
    }
    int err = SSL_get_error(_sslPtr->get(), r);
    if (err == SSL_ERROR_WANT_WRITE)
    {  // SSL want writable;
        _ioChannelPtr->enableWriting();
        _ioChannelPtr->disableReading();
    }
    else if (err == SSL_ERROR_WANT_READ)
    {  // SSL want readable;
        _ioChannelPtr->enableReading();
        _ioChannelPtr->disableWriting();
    }
    else
    {
        // ERR_print_errors(err);
        LOG_TRACE << "SSL handshake err: " << err;
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

    // send directly
    auto sendLen = SSL_write(_sslPtr->get(), buffer, length);
    if (sendLen <= 0)
    {
        int sslerr = SSL_get_error(_sslPtr->get(), sendLen);
        if (sslerr != SSL_ERROR_WANT_WRITE && sslerr != SSL_ERROR_WANT_READ)
        {
            LOG_ERROR << "ssl write error:" << sslerr;
            return -1;
        }
        return 0;
    }
    return sendLen;
}
