#pragma once

#include <trantor/net/TcpConnection.h>

namespace trantor
{


    class Channel;
    class Socket;
    class TcpServer;
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn);
    class TcpConnectionImpl:public TcpConnection,public NonCopyable,public std::enable_shared_from_this<TcpConnectionImpl>
    {
        friend class TcpServer;
        friend class TcpClient;
        friend void trantor::removeConnection(EventLoop* loop, const TcpConnectionPtr& conn);
    public:
        TcpConnectionImpl(EventLoop *loop,int socketfd,const InetAddress& localAddr,
                      const InetAddress& peerAddr);
        virtual ~TcpConnectionImpl();
        virtual void send(const char *msg,uint64_t len) override ;
        virtual void send(const std::string &msg) override;

        virtual const InetAddress& lobalAddr() const override{return localAddr_;}
        virtual const InetAddress& peerAddr() const override{return peerAddr_;}

        virtual bool connected() const override{ return state_ == Connected; }
        virtual bool disconnected() const override{ return state_ == Disconnected; }

        virtual MsgBuffer* getSendBuffer() override{ return  &writeBuffer_;}
        virtual MsgBuffer* getRecvBuffer() override{ return &readBuffer_;}
        //set callbacks
        virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t markLen)override{
            highWaterMarkCallback_=cb;
            highWaterMarkLen_=markLen;
        }

        virtual void setTcpNoDelay(bool on) override;
        virtual void shutdown() override;
        virtual void forceClose() override;
        virtual EventLoop* getLoop() override{return loop_;}

#ifdef NO_ANY
        virtual void setContext(void *p)const override{context_=p;}
        virtual void* getContext()const override{return context_;}
#else
        virtual void setContext(const any& context) override
        { context_ = context; }

        virtual const any& getContext() const override
        { return context_; }

        virtual any* getMutableContext() override
        { return &context_; }
#endif



    protected:
#ifdef NO_ANY
        mutable  void *context_= nullptr;
#else
        any context_;
#endif
        /// Internal use only.
        void setRecvMsgCallback(const RecvMessageCallback &cb){recvMsgCallback_=cb;}
        void setConnectionCallback(const ConnectionCallback &cb){connectionCallback_=cb;}
        void setWriteCompleteCallback(const WriteCompleteCallback &cb){writeCompleteCallback_=cb;}
        void setCloseCallback(const CloseCallback& cb)
        { closeCallback_ = cb; }
        void connectDestroyed();
        virtual void connectEstablished();

    protected:
        enum ConnState { Disconnected, Connecting, Connected, Disconnecting };
        EventLoop *loop_;
        std::unique_ptr<Channel> ioChennelPtr_;
        std::unique_ptr<Socket> socketPtr_;
        MsgBuffer readBuffer_;
        MsgBuffer writeBuffer_;

        virtual void readCallback();
        virtual void writeCallback();
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
        virtual void sendInLoop(const std::string &msg);

        size_t highWaterMarkLen_;
        std::string name_;
    };
}
