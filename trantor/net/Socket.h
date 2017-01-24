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

namespace trantor
{
    class Socket:NonCopyable
    {
    public:
        Socket(int family)
        {
            sockFd_ = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
            if (sockFd_ < 0)
            {
                LOG_SYSERR << "sockets::createNonblockingOrDie";
                exit(-1);
            }
        }
        ~Socket()
        {
            if(sockFd_>=0)
                close(sockFd_);
        }
        /// abort if address in use
        void bindAddress(const InetAddress& localaddr);
        /// abort if address in use
        void listen();
        int accept(InetAddress* peeraddr);
        void closeWrite();
        int fd(){return sockFd_;}
    protected:
        int sockFd_;

    };
}



#endif //TRANTOR_SOCKET_H
