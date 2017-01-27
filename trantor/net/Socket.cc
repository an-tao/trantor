//
// Created by antao on 2017/1/24.
//
#include <trantor/utils/Logger.h>
#include "Socket.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace trantor;
void Socket::bindAddress(const InetAddress &localaddr) {
    assert(sockFd_>0);
    int ret=bind(sockFd_,localaddr.getSockAddr(),sizeof(sockaddr));
    if(ret==0)
        return;
    else
    {
        LOG_SYSERR<<"bind failed";
        exit(-1);
    }
}
void Socket::listen() {
    assert(sockFd_>0);
    int ret = ::listen(sockFd_,SOMAXCONN);
    if(ret<0)
    {
        LOG_SYSERR<<"listen failed";
        exit(-1);
    }
}
int Socket::accept(InetAddress *peeraddr)
{

    struct sockaddr_in6 addr6;
    bzero(&addr6, sizeof addr6);
    socklen_t size=sizeof(addr6);
    int connfd = ::accept(sockFd_, (struct sockaddr *)&addr6,&size);
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet6(addr6);
    }
    return connfd;
}
void Socket::closeWrite() {
    if (::shutdown(sockFd_, SHUT_WR) < 0)
    {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}
int Socket::read(char *buffer, uint64_t len) {
    return ::read(sockFd_,buffer,len);
}