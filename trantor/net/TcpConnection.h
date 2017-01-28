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
        ~TcpConnection();
        void send(const char *msg,uint64_t len);
        void send(const std::string &&msg);

        void connectEstablished();
        bool connected() const { return state_ == Connected; }
        bool disconnected() const { return state_ == Disconnected; }
        //set callbacks
        void setRecvMsgCallback(const RecvMessageCallback &cb){recvMsgCallback_=cb;}
        void setConnectionCallback(const ConnectionCallback &cb){connectionCallback_=cb;}
        /// Internal use only.
        void setCloseCallback(const CloseCallback& cb)
        { closeCallback_ = cb; }
        void connectDestroyed();
    private:
        enum ConnState { Disconnected, Connecting, Connected, Disconnecting };
        EventLoop *loop_;
        std::unique_ptr<Channel> ioChennelPtr_;
        std::unique_ptr<Socket> socketPtr_;
        MsgBuffer readBuffer_;
        MsgBuffer writeBuffer_;

        void readCallback();
        void writeCallback();
        InetAddress localAddr_,peerAddr_;
        ConnState state_;
        //callbacks
        RecvMessageCallback recvMsgCallback_;
        ConnectionCallback connectionCallback_;
        CloseCallback closeCallback_;

        void handleClose();
        void sendInLoop(const std::string &msg);
    };
}