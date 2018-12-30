#pragma once

#include <trantor/utils/config.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/net/callbacks.h>
#include <memory>
#include <functional>
#include <string>

namespace trantor
{
class TcpConnection
{
  public:
    TcpConnection() = default;
    virtual ~TcpConnection(){};
    virtual void send(const char *msg, uint64_t len) = 0;
    virtual void send(const std::string &msg) = 0;
    virtual void send(std::string &&msg) = 0;
    virtual void send(const MsgBuffer &buffer) = 0;
    virtual void send(MsgBuffer &&buffer) = 0;
    virtual void send(const std::shared_ptr<std::string> &msgPtr) = 0;

    virtual void sendFile(const char *fileName, size_t offset = 0, size_t length = 0) = 0;

    virtual const InetAddress &localAddr() const = 0;
    virtual const InetAddress &peerAddr() const = 0;

    virtual bool connected() const = 0;
    virtual bool disconnected() const = 0;

    //virtual MsgBuffer* getSendBuffer() =0;
    virtual MsgBuffer *getRecvBuffer() = 0;
    //set callbacks
    virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t markLen) = 0;

    virtual void setTcpNoDelay(bool on) = 0;
    virtual void shutdown() = 0;
    virtual void forceClose() = 0;
    virtual EventLoop *getLoop() = 0;

    virtual void setContext(const any &context) = 0;
    virtual void setContext(any &&context) = 0;
    virtual const any &getContext() const = 0;

    virtual any *getMutableContext() = 0;

    //Call this method to avoid being kicked off by TcpServer, refer to 
    //the kickoffIdleConnections method in the TcpServer class. 
    virtual void keepAlive() = 0;
    virtual bool isKeepAlive() = 0;
};
} // namespace trantor
