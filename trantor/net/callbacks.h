#pragma once

#include <functional>
#include <memory>
namespace trantor{
    typedef std::function<void ()> TimerCallback;

// the data has been read to (buf, len)
    class TcpConnection;
    class MsgBuffer;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void (const TcpConnectionPtr&,
                                MsgBuffer*)> RecvMessageCallback;
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void (const TcpConnectionPtr&,const size_t)> HighWaterMarkCallback;
}
