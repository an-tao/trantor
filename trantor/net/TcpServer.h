#pragma once

#include <trantor/net/callbacks.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/net/TcpConnection.h>
#include <string>
#include <memory>
#include <set>
namespace trantor
{
    class Acceptor;

    class TcpServer:NonCopyable
    {
    public:
        TcpServer(EventLoop *loop,const InetAddress &address,const std::string &name);
        ~TcpServer();
        void start();
        void setRecvMessageCallback(const RecvMessageCallback &cb){recvMessageCallback_=cb;}
    protected:
        EventLoop *loop_;
        std::unique_ptr<Acceptor> acceptorPtr_;
        void newConnection(int fd,const InetAddress &peer);
        std::string serverName_;
        std::set<TcpConnectionPtr>connSet_;

        RecvMessageCallback recvMessageCallback_;
    };
}