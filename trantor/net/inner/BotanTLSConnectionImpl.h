#pragma once

#pragma once

#include <botan/tls_server.h>
#include <botan/tls_client.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>
#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/certstor_system.h>

#include <trantor/net/TcpConnection.h>

namespace trantor
{

class Client_Credentials : public Botan::Credentials_Manager
{
  public:
    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        // return a list of certificates of CAs we trust for tls server
        // certificates ownership of the pointers remains with
        // Credentials_Manager
        return {&m_cert_store};
    }

    std::vector<Botan::X509_Certificate> cert_chain(
        const std::vector<std::string> &cert_key_types,
        const std::string &type,
        const std::string &context) override
    {
        // when using tls client authentication (optional), return
        // a certificate chain being sent to the tls server,
        // else an empty list
        return {};
    }

    Botan::Private_Key *private_key_for(const Botan::X509_Certificate &cert,
                                        const std::string &type,
                                        const std::string &context) override
    {
        // when returning a chain in cert_chain(), return the private key
        // associated with the leaf certificate here
        return nullptr;
    }

    // void tls_verify_cert_chain(
    //       const std::vector<Botan::X509_Certificate>& cert_chain,
    //       const std::vector<std::shared_ptr<const Botan::OCSP::Response>>&
    //       ocsp_responses, const std::vector<Botan::Certificate_Store*>&
    //       trusted_roots, Botan::Usage_Type usage, const std::string&
    //       hostname, const Botan::TLS::Policy& policy)
    // {

    // }

  private:
    Botan::System_Certificate_Store m_cert_store;
};

class TestPolicy : public Botan::TLS::Policy
{
    bool require_cert_revocation_info() const override
    {
        return false;
    }
};

class BotanTLSConnectionImpl
    : public TcpConnection,
      public NonCopyable,
      public Botan::TLS::Callbacks,
      public std::enable_shared_from_this<BotanTLSConnectionImpl>
{
  public:
    BotanTLSConnectionImpl(TcpConnectionPtr rawConn);
    virtual ~BotanTLSConnectionImpl(){};
    virtual void send(const char *msg, size_t len) override;
    virtual void send(const void *msg, size_t len) override
    {
        send(static_cast<const char *>(msg), len);
    }
    virtual void send(const std::string &msg) override
    {
        send(msg.data(), msg.size());
    }
    virtual void send(std::string &&msg) override
    {
        send(msg.data(), msg.size());
    }
    virtual void send(const MsgBuffer &buffer) override
    {
        send(buffer.peek(), buffer.readableBytes());
    }
    virtual void send(MsgBuffer &&buffer) override
    {
        send(buffer.peek(), buffer.readableBytes());
    }
    virtual void send(const std::shared_ptr<std::string> &msgPtr) override
    {
        send(msgPtr->data(), msgPtr->size());
    }
    virtual void send(const std::shared_ptr<MsgBuffer> &msgPtr) override
    {
        send(msgPtr->peek(), msgPtr->readableBytes());
    }
    virtual void sendFile(const char *fileName,
                          size_t offset = 0,
                          size_t length = 0) override
    {
        throw std::runtime_error("Not implemented");
    }
    virtual void sendFile(const wchar_t *fileName,
                          size_t offset = 0,
                          size_t length = 0) override
    {
        throw std::runtime_error("Not implemented");
    }
    virtual void sendStream(
        std::function<std::size_t(char *, std::size_t)> callback) override
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const InetAddress &localAddr() const override
    {
        return rawConnPtr_->localAddr();
    }
    virtual const InetAddress &peerAddr() const override
    {
        return rawConnPtr_->peerAddr();
    }

    // TODO: add state to these functions
    virtual bool connected() const override
    {
        return rawConnPtr_->connected() && !closingTLS_;
    }
    virtual bool disconnected() const override
    {
        return rawConnPtr_->disconnected() || closingTLS_;
    }

    // virtual MsgBuffer *getRecvBuffer() override { throw
    // std::runtime_error("Not implemented"); }
    virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                          size_t markLen) override
    {
        throw std::runtime_error("Not implemented");
    }

    virtual void keepAlive() override
    {
        rawConnPtr_->keepAlive();
    }
    virtual bool isKeepAlive() override
    {
        return rawConnPtr_->isKeepAlive();
    }

    virtual void setTcpNoDelay(bool on) override
    {
        rawConnPtr_->setTcpNoDelay(on);
    }

    virtual void shutdown() override;
    virtual void forceClose() override;

    virtual EventLoop *getLoop() override
    {
        return rawConnPtr_->getLoop();
    }
    virtual size_t bytesSent() const override
    {
        return rawConnPtr_->bytesSent();
    }
    virtual size_t bytesReceived() const override
    {
        return rawConnPtr_->bytesReceived();
    }
    virtual bool isSSLConnection() const override
    {
        return true;
    }

    // TODO: Get rid of these functions
    virtual void startClientEncryption(
        std::function<void()> callback,
        bool useOldTLS = false,
        bool validateCert = true,
        std::string hostname = "",
        const std::vector<std::pair<std::string, std::string>> &sslConfCmds =
            {});
    virtual void startServerEncryption(const std::shared_ptr<SSLContext> &ctx,
                                       std::function<void()> callback) override
    {
        throw std::runtime_error("Not implemented");
    }

  protected:
    TcpConnectionPtr rawConnPtr_;

    void onRecvMessage(const TcpConnectionPtr &conn, MsgBuffer *buffer);
    void onConnection(const TcpConnectionPtr &conn);
    void onWriteComplete(const TcpConnectionPtr &conn);
    void onDisconnection(const TcpConnectionPtr &conn);
    void onClosed(const TcpConnectionPtr &conn);

    virtual void connectEstablished() override
    {
        rawConnPtr_->connectEstablished();
    }
    virtual void connectDestroyed() override
    {
        rawConnPtr_->connectDestroyed();
    }

    void tls_emit_data(const uint8_t data[], size_t size) override;
    void tls_record_received(uint64_t seq_no,
                             const uint8_t data[],
                             size_t size) override;
    void tls_alert(Botan::TLS::Alert alert) override;
    bool tls_session_established(const Botan::TLS::Session &session) override;

    Botan::AutoSeeded_RNG rng_;
    Botan::TLS::Session_Manager_In_Memory sessionManager_;
    TestPolicy policy_;
    Client_Credentials creds_;
    std::unique_ptr<Botan::TLS::Client> client_;
    MsgBuffer recvBuffer_;

    bool closingTLS_ = false;
};
}  // namespace trantor