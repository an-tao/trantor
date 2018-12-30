#pragma once

#include <trantor/net/TcpConnection.h>
#include <trantor/utils/TimingWheel.h>
#include <list>
#include <unistd.h>
#include <thread>

namespace trantor
{

class Channel;
class Socket;
class TcpServer;
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn);
class TcpConnectionImpl : public TcpConnection, public NonCopyable, public std::enable_shared_from_this<TcpConnectionImpl>
{
    friend class TcpServer;
    friend class TcpClient;
    friend void trantor::removeConnection(EventLoop *loop, const TcpConnectionPtr &conn);

  public:
    class KickoffEntry
    {
      public:
        KickoffEntry(const std::weak_ptr<TcpConnection> &conn) : _conn(conn) {}
        void reset()
        {
            _conn.reset();
        }
        ~KickoffEntry()
        {
            auto conn = _conn.lock();
            if (conn)
            {
                conn->forceClose();
            }
        }

      private:
        std::weak_ptr<TcpConnection> _conn;
    };

    TcpConnectionImpl(EventLoop *loop, int socketfd, const InetAddress &localAddr,
                      const InetAddress &peerAddr);
    virtual ~TcpConnectionImpl();
    virtual void send(const char *msg, uint64_t len) override;
    virtual void send(const std::string &msg) override;
    virtual void send(std::string &&msg) override;
    virtual void send(const MsgBuffer &buffer) override;
    virtual void send(MsgBuffer &&buffer) override;
    virtual void send(const std::shared_ptr<std::string> &msgPtr) override;
    virtual void sendFile(const char *fileName, size_t offset = 0, size_t length = 0) override;

    virtual const InetAddress &localAddr() const override { return localAddr_; }
    virtual const InetAddress &peerAddr() const override { return peerAddr_; }

    virtual bool connected() const override { return state_ == Connected; }
    virtual bool disconnected() const override { return state_ == Disconnected; }

    //virtual MsgBuffer* getSendBuffer() override{ return  &writeBuffer_;}
    virtual MsgBuffer *getRecvBuffer() override { return &readBuffer_; }
    //set callbacks
    virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t markLen) override
    {
        highWaterMarkCallback_ = cb;
        highWaterMarkLen_ = markLen;
    }

    virtual void keepAlive() override
    {
        _idleTimeout = 0;
        auto entry = _kickoffEntry.lock();
        if (entry)
        {
            entry->reset();
        }
    }
    virtual bool isKeepAlive() override { return _idleTimeout == 0; }
    virtual void setTcpNoDelay(bool on) override;
    virtual void shutdown() override;
    virtual void forceClose() override;
    virtual EventLoop *getLoop() override { return loop_; }

    virtual void setContext(const any &context) override
    {
        context_ = context;
    }
    virtual void setContext(any &&context) override
    {
        context_ = std::move(context);
    }
    virtual const any &getContext() const override
    {
        return context_;
    }

    virtual any *getMutableContext() override
    {
        return &context_;
    }

  protected:
    any context_;
    /// Internal use only.

    std::weak_ptr<KickoffEntry> _kickoffEntry;
    std::shared_ptr<TimingWheel> _timingWheelPtr;
    size_t _idleTimeout = 0;
    Date _lastTimingWheelUpdateTime;

    void enableKickingOff(size_t timeout, const std::shared_ptr<TimingWheel> &timingWheel)
    {
        assert(timingWheel);
        assert(timingWheel->getLoop() == loop_);
        assert(timeout > 0);
        auto entry = std::make_shared<KickoffEntry>(shared_from_this());
        _kickoffEntry = entry;
        _timingWheelPtr = timingWheel;
        _idleTimeout = timeout;
        _timingWheelPtr->insertEntry(timeout, entry);
    }
    void extendLife();
    void sendFile(int sfd, size_t offset = 0, size_t length = 0);
    void setRecvMsgCallback(const RecvMessageCallback &cb) { recvMsgCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    void connectDestroyed();
    virtual void connectEstablished();
    bool _isSSLConn = false;

  protected:
    struct BufferNode
    {
        int _sendFd = -1;
        ssize_t _fileBytesToSend;
        off_t _offset;
        std::shared_ptr<MsgBuffer> _msgBuffer;
        ~BufferNode()
        {
            if (_sendFd >= 0)
                close(_sendFd);
        }
    };
    typedef std::shared_ptr<BufferNode> BufferNodePtr;
    enum ConnState
    {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };
    EventLoop *loop_;
    std::unique_ptr<Channel> ioChennelPtr_;
    std::unique_ptr<Socket> socketPtr_;
    MsgBuffer readBuffer_;
    //MsgBuffer writeBuffer_;
    std::list<BufferNodePtr> _writeBufferList;
    virtual void readCallback();
    virtual void writeCallback();
    InetAddress localAddr_, peerAddr_;
    ConnState state_;
    //callbacks
    RecvMessageCallback recvMsgCallback_;
    ConnectionCallback connectionCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    void handleClose();
    void handleError();
    //virtual void sendInLoop(const std::string &msg);
    void sendInLoop(const char *buffer, size_t length);

    void sendFileInLoop(const BufferNodePtr &file);

    virtual ssize_t writeInLoop(const char *buffer, size_t length);
    size_t highWaterMarkLen_;
    std::string name_;

    uint64_t _sendNum = 0;
    std::mutex _sendNumMutex;
};
} // namespace trantor
