#pragma once

#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/net/callbacks.h>
#include <memory>
#include <functional>
namespace trantor
{
    class Channel;
    class Socket;
    class TcpConnection:public NonCopyable,public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(EventLoop *loop,int socketfd,const InetAddress& localAddr,
                      const InetAddress& peerAddr);
        void send(const char *msg,uint64_t len){};
        void send(const std::string &msg){};
        void setRecvMsgCallback(const RecvMessageCallback &cb){recvMsgCallback_=cb;}
    protected:
        EventLoop *loop_;
        std::unique_ptr<Channel> ioChennelPtr_;
        int sockfd_;
        MsgBuffer buffer_;
        RecvMessageCallback recvMsgCallback_;
        void readCallback();
        InetAddress localAddr_,peerAddr_;
    };
}