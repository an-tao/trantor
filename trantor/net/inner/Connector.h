#pragma once

#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <atomic>
#include <memory>

namespace trantor
{

class Connector : public NonCopyable, public std::enable_shared_from_this<Connector>
{
  public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;
    typedef std::function<void()> ConnectionErrorCallback;
    Connector(EventLoop *loop, const InetAddress &addr, bool retry = true);
    Connector(EventLoop *loop, InetAddress &&addr, bool retry = true);
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }
    void setNewConnectionCallback(NewConnectionCallback &&cb)
    {
        newConnectionCallback_ = std::move(cb);
    }
    void setErrorCallback(const ConnectionErrorCallback &cb)
    {
        _errorCallback = cb;
    }
    void setErrorCallback(ConnectionErrorCallback &&cb)
    {
        _errorCallback = std::move(cb);
    }
    const InetAddress &serverAddress() const { return _serverAddr; }
    void start();
    void restart();
    void stop();

  private:
    NewConnectionCallback newConnectionCallback_;
    ConnectionErrorCallback _errorCallback;
    enum States
    {
        kDisconnected,
        kConnecting,
        kConnected
    };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;
    std::shared_ptr<Channel> _channelPtr;
    EventLoop *_loop;
    InetAddress _serverAddr;

    std::atomic_bool _connect;
    std::atomic<States> _state;

    int _retryInterval;
    int _maxRetryInterval;

    bool _retry;

    void startInLoop();
    void connect();
    void connecting(int sockfd);
    int removeAndResetChannel();
    void handleWrite();
    void handleError();
    void retry(int sockfd);
};

} // namespace trantor
