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
    class TcpServer;
    class TcpConnection:public NonCopyable,public std::enable_shared_from_this<TcpConnection>
    {
        friend class TcpServer;
        friend class TcpClient;
        friend void trantor::removeConnection(EventLoop* loop, const TcpConnectionPtr& conn);
    public:
        TcpConnection(EventLoop *loop,int socketfd,const InetAddress& localAddr,
                      const InetAddress& peerAddr);
        ~TcpConnection();
        void send(const char *msg,uint64_t len);
        void send(const std::string &msg);

        const InetAddress& lobalAddr() const {return localAddr_;}
        const InetAddress& peerAddr() const {return peerAddr_;}

        bool connected() const { return state_ == Connected; }
        bool disconnected() const { return state_ == Disconnected; }

        MsgBuffer* getSendBuffer() { return  &writeBuffer_;}
        MsgBuffer* getRecvBuffer() { return &readBuffer_;}
        //set callbacks
        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t markLen){
            highWaterMarkCallback_=cb;
            highWaterMarkLen_=markLen;
        }

        void setTcpNoDelay(bool on);
        void shutdown();
        void forceClose();
        EventLoop* getLoop(){return loop_;}


    protected:
        /// Internal use only.
        void setRecvMsgCallback(const RecvMessageCallback &cb){recvMsgCallback_=cb;}
        void setConnectionCallback(const ConnectionCallback &cb){connectionCallback_=cb;}
        void setWriteCompleteCallback(const WriteCompleteCallback &cb){writeCompleteCallback_=cb;}
        void setCloseCallback(const CloseCallback& cb)
        { closeCallback_ = cb; }
        void connectDestroyed();
        void connectEstablished();
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
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        void handleClose();
        void handleError();
        void sendInLoop(const std::string &msg);

        size_t highWaterMarkLen_;
        std::string name_;
    };
}