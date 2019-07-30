/**
 *
 *  SSLConnectionSkipped.cc
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
#include <thread>

using namespace trantor;

namespace trantor
{
class SSLContext
{
};
class SSLConn
{
};

void initOpenSSL()
{
    LOG_FATAL << "SSL is not supported due to not being compiled with OpenSSL!";
    exit(1);
}
std::shared_ptr<SSLContext> newSSLContext()
{
    return nullptr;
}
void initServerSSLContext(const std::shared_ptr<SSLContext> &ctx,
                          const std::string &certPath,
                          const std::string &keyPath)
{
    LOG_FATAL << "SSL is not supported due to not being compiled with OpenSSL!";
    exit(1);
}
}  // namespace trantor

SSLConnection::SSLConnection(EventLoop *loop,
                             int socketfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr,
                             const std::shared_ptr<SSLContext> &ctxPtr,
                             bool isServer)
    : TcpConnectionImpl(loop, socketfd, localAddr, peerAddr)
{
    LOG_FATAL << "SSL is not supported due to not being compiled with OpenSSL!";
    exit(1);
}

void SSLConnection::writeCallback()
{
}

void SSLConnection::readCallback()
{
}

void SSLConnection::connectEstablished()
{
}
void SSLConnection::doHandshaking()
{
}

ssize_t SSLConnection::writeInLoop(const char *buffer, size_t length)
{
    return -1;
}
