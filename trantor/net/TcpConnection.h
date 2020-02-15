/**
 *
 *  TcpConnection.h
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#pragma once
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
class SSLContext;
std::shared_ptr<SSLContext> newSSLServerContext(const std::string &certPath,
                                                const std::string &keyPath);
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

    virtual void sendFile(const char *fileName,
                          size_t offset = 0,
                          size_t length = 0) = 0;

    virtual const InetAddress &localAddr() const = 0;
    virtual const InetAddress &peerAddr() const = 0;

    virtual bool connected() const = 0;
    virtual bool disconnected() const = 0;

    // virtual MsgBuffer* getSendBuffer() =0;
    virtual MsgBuffer *getRecvBuffer() = 0;
    // set callbacks
    virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                          size_t markLen) = 0;

    virtual void setTcpNoDelay(bool on) = 0;
    virtual void shutdown() = 0;
    virtual void forceClose() = 0;
    virtual EventLoop *getLoop() = 0;

    /// Set custom data on the connection
    void setContext(const std::shared_ptr<void> &context)
    {
        contextPtr_ = context;
    }
    void setContext(std::shared_ptr<void> &&context)
    {
        contextPtr_ = std::move(context);
    }

    /// Get custom data from the connection
    template <typename T>
    std::shared_ptr<T> getContext() const
    {
        return std::static_pointer_cast<T>(contextPtr_);
    }

    /// Return true if the context is set by user.
    bool hasContext() const
    {
        return (bool)contextPtr_;
    }

    /// Clear the context.
    void clearContext()
    {
        contextPtr_.reset();
    }

    // Call this method to avoid being kicked off by TcpServer, refer to
    // the kickoffIdleConnections method in the TcpServer class.
    virtual void keepAlive() = 0;
    virtual bool isKeepAlive() = 0;

    virtual size_t bytesSent() const = 0;
    virtual size_t bytesReceived() const = 0;

    virtual bool isSSLConnection() const = 0;

    virtual void startClientEncryption(std::function<void()> callback) = 0;
    virtual void startServerEncryption(const std::shared_ptr<SSLContext> &ctx,
                                       std::function<void()> callback) = 0;

  private:
    std::shared_ptr<void> contextPtr_;
};

}  // namespace trantor
