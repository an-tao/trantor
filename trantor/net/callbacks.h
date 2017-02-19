#pragma once

#include <functional>
#include <memory>
namespace trantor{
    enum TrantorError{
        TrantorError_None,
        TrantorError_UnkownError
    };
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

    typedef std::function<void (const TrantorError)> OperationCompleteCallback;

    void defaultConnectionCallback(const TcpConnectionPtr& conn);
    void defaultMessageCallback(const TcpConnectionPtr& conn,
                                MsgBuffer* buffer);

}
