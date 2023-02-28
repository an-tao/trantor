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
    TLSProvider(TcpConnection* conn, TLSPolicyPtr policy, SSLContextPtr ctx)
        : conn_(conn),
          policyPtr_(std::move(policy)),
          loop_(conn_->getLoop()),
          contextPtr_(std::move(ctx))
    {
    }
    virtual ~TLSProvider() = default;
    using WriteCallback = void (*)(TcpConnection*,
                                   const void* data,
                                   size_t len);
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
    void setWriteCallback(WriteCallback cb)
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

    MsgBuffer& getRecvBuffer()
    {
        return recvBuffer_;
    }

    const CertificatePtr& peerCertificate() const
    {
        return peerCertificate_;
    }

    const std::string& applicationProtocol() const
    {
        return applicationProtocol_;
    }

    const std::string& sniName() const
    {
        return sniName_;
    }

  protected:
    void setPeerCertificate(CertificatePtr cert)
    {
        peerCertificate_ = std::move(cert);
    }

    void setApplicationProtocol(std::string protocol)
    {
        applicationProtocol_ = std::move(protocol);
    }

    void setSniName(std::string name)
    {
        sniName_ = std::move(name);
    }

    WriteCallback writeCallback_ = nullptr;
    ErrorCallback errorCallback_ = nullptr;
    HandshakeCallback handshakeCallback_ = nullptr;
    MessageCallback messageCallback_ = nullptr;
    CloseCallback closeCallback_ = nullptr;
    TcpConnection* conn_ = nullptr;
    const TLSPolicyPtr policyPtr_;
    const SSLContextPtr contextPtr_;
    MsgBuffer recvBuffer_;
    EventLoop* loop_ = nullptr;
    CertificatePtr peerCertificate_;
    std::string applicationProtocol_;
    std::string sniName_;
};

std::unique_ptr<TLSProvider> newTLSProvider(TcpConnection* conn,
                                            TLSPolicyPtr policy,
                                            SSLContextPtr ctx);
}  // namespace trantor