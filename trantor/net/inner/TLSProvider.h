#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/net/callbacks.h>
#include <trantor/net/TcpConnection.h>

#include <memory>

namespace trantor
{
struct TLSProvider
{
    TLSProvider(EventLoop* loop, TcpConnection* conn, TLSPolicyPtr policy)
        : conn_(conn), policy_(std::move(policy)), loop_(loop)
    {
    }
    virtual ~TLSProvider() = default;
    using WtriteCallback = void (*)(TcpConnection*, const MsgBuffer& buffer);
    using ErrorCallback = void (*)(TcpConnection*, SSLError err);
    using HandshakeCallback = void (*)(TcpConnection*);
    using MessageCallback = void (*)(TcpConnection*, MsgBuffer* buffer);
    using CloseCallback = void (*)(TcpConnection*);

    /**
     * @brief Sends data to the TLSProvider to process handshake and decrypt
     * data
     */
    virtual void recvData(MsgBuffer* buffer) = 0;

    /**
     * @brief Encrypt and send data via TLS
     */
    virtual void sendData(const char* ptr, size_t size) = 0;

    virtual void startEncryption() = 0;

    /**
     * @brief Set a function to be called when the TLSProvider wants to send
     * data
     *
     * @note The caller MUST guarantee that it will not make the TLSProvider
     * send data after caller is destroyed. std::function used due to
     * performance reasons.
     */
    void setWriteCallback(WtriteCallback cb)
    {
        writeCallback_ = cb;
    }

    void setErrorCallback(ErrorCallback cb)
    {
        errorCallback_ = cb;
    }

    void setHandshakeCallback(HandshakeCallback cb)
    {
        handshakeCallback_ = cb;
    }

    void setMessageCallback(MessageCallback cb)
    {
        messageCallback_ = cb;
    }
    void setCloseCallback(CloseCallback cb)
    {
        closeCallback_ = cb;
    }

  protected:
    WtriteCallback writeCallback_ = nullptr;
    ErrorCallback errorCallback_ = nullptr;
    HandshakeCallback handshakeCallback_ = nullptr;
    MessageCallback messageCallback_ = nullptr;
    CloseCallback closeCallback_ = nullptr;
    TcpConnection* conn_ = nullptr;
    TLSPolicyPtr policy_;
    EventLoop* loop_ = nullptr;
};

std::unique_ptr<TLSProvider> newTLSProvider(EventLoop* loop,
                                            TcpConnection* conn,
                                            TLSPolicyPtr policy,
                                            SSLContextPtr ctx);
}  // namespace trantor