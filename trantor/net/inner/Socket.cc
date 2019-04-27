/**
 *
 *  Socket.cc
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

#include <trantor/utils/Logger.h>
#include "Socket.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

using namespace trantor;
void Socket::bindAddress(const InetAddress &localaddr)
{
    assert(sockFd_ > 0);
    int ret;
    if (localaddr.isIpV6())
        ret = ::bind(sockFd_, localaddr.getSockAddr(), sizeof(sockaddr_in6));
    else
        ret = ::bind(sockFd_, localaddr.getSockAddr(), sizeof(sockaddr_in));

    if (ret == 0)
        return;
    else
    {
        LOG_SYSERR << ", Bind address failed at " << localaddr.toIpPort();
        exit(-1);
    }
}
void Socket::listen()
{
    assert(sockFd_ > 0);
    int ret = ::listen(sockFd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSERR << "listen failed";
        exit(-1);
    }
}
int Socket::accept(InetAddress *peeraddr)
{

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    socklen_t size = sizeof(addr6);
#ifdef __linux__
    int connfd = ::accept4(sockFd_, (struct sockaddr *)&addr6,
                           &size, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int connfd = ::accept(sockFd_, (struct sockaddr *)&addr6, &size);
    setNonBlockAndCloseOnExec(connfd);
#endif
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet6(addr6);
    }
    return connfd;
}
void Socket::closeWrite()
{
    if (::shutdown(sockFd_, SHUT_WR) < 0)
    {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}
int Socket::read(char *buffer, uint64_t len)
{
    return ::read(sockFd_, buffer, len);
}

struct sockaddr_in6 Socket::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, static_cast<struct sockaddr *>((void *)(&localaddr)), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in6 Socket::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, static_cast<struct sockaddr *>((void *)(&peeraddr)), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockFd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockFd_, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // TODO CHECK
}

int Socket::getSocketError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockFd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}
