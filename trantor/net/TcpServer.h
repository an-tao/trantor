#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <string>
#include <memory>
namespace trantor
{
    class Acceptor;

    class TcpServer:NonCopyable
    {
    public:
        TcpServer(EventLoop *loop,const InetAddress &address,const std::string &name);
        ~TcpServer();
        void start();
    protected:
        EventLoop *loop_;
        std::unique_ptr<Acceptor> acceptorPtr_;
        void newConnection(int fd,const InetAddress &peer);
        std::string serverName_;

    };
}