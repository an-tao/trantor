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
        ctxPtr_ = SSL_CTX_new(SSLv23_method());
    }
    ~SSLContext()
    {
        if (ctxPtr_)
        {
            SSL_CTX_free(ctxPtr_);
        }
    }

    SSL_CTX *get()
    {
        return ctxPtr_;
    }

  private:
    SSL_CTX *ctxPtr_;
};
class SSLConn
{
  public:
    explicit SSLConn(SSL_CTX *ctx)
    {
        SSL_ = SSL_new(ctx);
    }
    ~SSLConn()
    {
        if (SSL_)
        {
            SSL_free(SSL_);
        }
    }
    SSL *get()
    {
        return SSL_;
    }

  private:
    SSL *SSL_;
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
      sslPtr_(std::make_shared<SSLConn>(ctxPtr->get())),
      isServer_(isServer)
{
    assert(sslPtr_);
    auto r = SSL_set_fd(sslPtr_->get(), socketfd);
    (void)r;
    assert(r);
    isSSLConn_ = true;
}

void SSLConnection::writeCallback()
{
    LOG_TRACE << "write Callback";
    loop_->assertInLoopThread();
    if (statusOfSSL_ == SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if (statusOfSSL_ == SSLStatus::Connected)
    {
        TcpConnectionImpl::writeCallback();
    }
}

void SSLConnection::readCallback()
{
    LOG_TRACE << "read Callback";
    loop_->assertInLoopThread();
    if (statusOfSSL_ == SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if (statusOfSSL_ == SSLStatus::Connected)
    {
        int rd;
        bool newDataFlag = false;
        size_t readLength;
        do
        {
            readBuffer_.ensureWritableBytes(1024);
            readLength = readBuffer_.writableBytes();
            rd = SSL_read(sslPtr_->get(), readBuffer_.beginWrite(), readLength);
            LOG_TRACE << "ssl read:" << rd << " bytes";
            if (rd <= 0)
            {
                int sslerr = SSL_get_error(sslPtr_->get(), rd);
                if (sslerr == SSL_ERROR_WANT_READ)
                {
                    break;
                }
                else
                {
                    LOG_TRACE << "ssl read err:" << sslerr;
                    statusOfSSL_ = SSLStatus::DisConnected;
                    handleClose();
                    return;
                }
            }
            readBuffer_.hasWritten(rd);
            newDataFlag = true;
        } while ((size_t)rd == readLength);
        if (newDataFlag)
        {
            // Run callback function
            recvMsgCallback_(shared_from_this(), &readBuffer_);
        }
    }
}

void SSLConnection::connectEstablished()
{
    loop_->runInLoop([=]() {
        LOG_TRACE << "connectEstablished";
        assert(status_ == ConnStatus::Connecting);
        ioChannelPtr_->tie(shared_from_this());
        ioChannelPtr_->enableReading();
        status_ = ConnStatus::Connected;
        if (isServer_)
        {
            SSL_set_accept_state(sslPtr_->get());
        }
        else
        {
            ioChannelPtr_->enableWriting();
            SSL_set_connect_state(sslPtr_->get());
        }
    });
}
void SSLConnection::doHandshaking()
{
    assert(statusOfSSL_ == SSLStatus::Handshaking);
    int r = SSL_do_handshake(sslPtr_->get());
    if (r == 1)
    {
        statusOfSSL_ = SSLStatus::Connected;
        connectionCallback_(shared_from_this());
        return;
    }
    int err = SSL_get_error(sslPtr_->get(), r);
    if (err == SSL_ERROR_WANT_WRITE)
    {  // SSL want writable;
        ioChannelPtr_->enableWriting();
        ioChannelPtr_->disableReading();
    }
    else if (err == SSL_ERROR_WANT_READ)
    {  // SSL want readable;
        ioChannelPtr_->enableReading();
        ioChannelPtr_->disableWriting();
    }
    else
    {
        // ERR_print_errors(err);
        LOG_TRACE << "SSL handshake err: " << err;
        ioChannelPtr_->disableReading();
        statusOfSSL_ = SSLStatus::DisConnected;
        forceClose();
    }
}

ssize_t SSLConnection::writeInLoop(const char *buffer, size_t length)
{
    LOG_TRACE << "send in loop";
    loop_->assertInLoopThread();
    if (status_ != ConnStatus::Connected)
    {
        LOG_WARN << "Connection is not connected,give up sending";
        return -1;
    }
    if (statusOfSSL_ != SSLStatus::Connected)
    {
        LOG_WARN << "SSL is not connected,give up sending";
        return -1;
    }
    // send directly
    size_t sendTotalLen = 0;
    while (sendTotalLen < length)
    {
        auto len = length - sendTotalLen;
        if (len > sendBuffer_.size())
        {
            len = sendBuffer_.size();
        }
        memcpy(sendBuffer_.data(), buffer + sendTotalLen, len);
        ERR_clear_error();
        auto sendLen = SSL_write(sslPtr_->get(), sendBuffer_.data(), len);
        if (sendLen <= 0)
        {
            int sslerr = SSL_get_error(sslPtr_->get(), sendLen);
            if (sslerr != SSL_ERROR_WANT_WRITE && sslerr != SSL_ERROR_WANT_READ)
            {
                //LOG_ERROR << "ssl write error:" << sslerr;
                forceClose();
                return -1;
            }
            return sendTotalLen;
        }
        sendTotalLen += sendLen;
    }
    return sendTotalLen;
}
