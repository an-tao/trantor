//
// Created by antao on 2017/1/24.
//

#ifndef TRANTOR_SOCKET_H
#define TRANTOR_SOCKET_H
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/InetAddress.h>
#include <trantor/utils/Logger.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>

namespace trantor
{
    class Socket:NonCopyable
    {
    public:
        static int createNonblockingSocketOrDie(int family)
        {
            int sock = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
            if (sock < 0)
            {
                LOG_SYSERR << "sockets::createNonblockingOrDie";
                exit(-1);
            }
            return sock;
        }
        //token from muduo
        static void setNonBlockAndCloseOnExec(int sockfd)
        {
            // non-block
            int flags = ::fcntl(sockfd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            int ret = ::fcntl(sockfd, F_SETFL, flags);
            // FIXME check

            // close-on-exec
            flags = ::fcntl(sockfd, F_GETFD, 0);
            flags |= FD_CLOEXEC;
            ret = ::fcntl(sockfd, F_SETFD, flags);
            // FIXME check

            (void)ret;
        }
        explicit Socket(int sockfd):
                sockFd_(sockfd)
        {}
        ~Socket()
        {
            LOG_TRACE<<"Socket deconstructed";
            if(sockFd_>=0)
                close(sockFd_);
        }
        /// abort if address in use
        void bindAddress(const InetAddress& localaddr);
        /// abort if address in use
        void listen();
        int accept(InetAddress* peeraddr);
        void closeWrite();
        int read(char *buffer,uint64_t len);
        int fd(){return sockFd_;}
        static struct sockaddr_in6 getLocalAddr(int sockfd);
        static struct sockaddr_in6 getPeerAddr(int sockfd);

        ///
        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        ///
        void setTcpNoDelay(bool on);

        ///
        /// Enable/disable SO_REUSEADDR
        ///
        void setReuseAddr(bool on);

        ///
        /// Enable/disable SO_REUSEPORT
        ///
        void setReusePort(bool on);

        ///
        /// Enable/disable SO_KEEPALIVE
        ///
        void setKeepAlive(bool on);
        int getSocketError();
    protected:
        int sockFd_;

    };
}



#endif //TRANTOR_SOCKET_H
