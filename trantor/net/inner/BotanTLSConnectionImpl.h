#pragma once

#pragma once

#include <botan/tls_server.h>
#include <botan/tls_client.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>
#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/certstor_system.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>

#include <trantor/net/TcpConnection.h>
#include <trantor/utils/Logger.h>

namespace trantor
{
class ClientCredentials : public Botan::Credentials_Manager
{
  public:
    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        // return a list of certificates of CAs we trust for tls server
        // certificates ownership of the pointers remains with
        // Credentials_Manager
        return {&certStore_};
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
    Botan::System_Certificate_Store certStore_;
};

class ServerCredentials : public Botan::Credentials_Manager
{
  public:
    ServerCredentials(const std::string &keyFile, const std::string &certFile)
    {
        Botan::DataSource_Stream in(keyFile);
        key_ = Botan::PKCS8::load_key(in);
        cert_ = std::make_unique<Botan::X509_Certificate>(certFile);
    }

    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        // if client authentication is required, this function
        // shall return a list of certificates of CAs we trust
        // for tls client certificates, otherwise return an empty list
        return {};
    }

    std::vector<Botan::X509_Certificate> cert_chain(
        const std::vector<std::string> &cert_key_types,
        const std::string &type,
        const std::string &context) override
    {
        // return the certificate chain being sent to the tls client
        // e.g., the certificate file "botan.randombit.net.crt"
        return {*cert_};
    }

    Botan::Private_Key *private_key_for(const Botan::X509_Certificate &cert,
                                        const std::string &type,
                                        const std::string &context) override
    {
        // return the private key associated with the leaf certificate,
        // in this case the one associated with "botan.randombit.net.crt"
        return key_.get();
    }

  private:
    std::unique_ptr<Botan::Private_Key> key_;
    std::unique_ptr<Botan::X509_Certificate> cert_;
};

struct BotanCertificate : public Certificate
{
    BotanCertificate(const Botan::X509_Certificate &cert) : cert_(cert)
    {
    }

    virtual std::string sha1Fingerprint() const override
    {
        return cert_.fingerprint("SHA-1");
    }

    virtual std::string sha256Fingerprint() const override
    {
        return cert_.fingerprint("SHA-256");
    }

    virtual std::string pem() const override
    {
        return cert_.PEM_encode();
    }
    Botan::X509_Certificate cert_;
};

class BotanTLSConnectionImpl
    : public TcpConnection,
      public NonCopyable,
      public Botan::TLS::Callbacks,
      public std::enable_shared_from_this<BotanTLSConnectionImpl>
{
  public:
    BotanTLSConnectionImpl(TcpConnectionPtr rawConn, SSLPolicyPtr policy);
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

    virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                          size_t markLen) override;

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

    virtual MsgBuffer *getRecvBuffer() override
    {
        return &recvBuffer_;
    }

    virtual CertificatePtr peerCertificate() const override
    {
        return peerCertPtr_;
    }

    virtual std::string sniName() const override
    {
        return sniName_;
    }

    virtual std::string applicationProtocol() const override;
    void startClientEncryption();
    void startServerEncryption();

  protected:
    TcpConnectionPtr rawConnPtr_;

    void onRecvMessage(const TcpConnectionPtr &conn, MsgBuffer *buffer);
    void onConnection(const TcpConnectionPtr &conn);
    void onWriteComplete(const TcpConnectionPtr &conn);
    void onDisconnection(const TcpConnectionPtr &conn);
    void onClosed(const TcpConnectionPtr &conn);
    void onHighWaterMark(const TcpConnectionPtr &conn, size_t markLen);

    virtual void connectEstablished() override
    {
        rawConnPtr_->connectEstablished();
    }
    virtual void connectDestroyed() override
    {
        rawConnPtr_->connectDestroyed();
    }

    void enableKickingOff(size_t timeout,
                          const std::shared_ptr<TimingWheel> &timingWheel)
    {
        rawConnPtr_->enableKickingOff(timeout, timingWheel);
    }

    void handleSSLError(SSLError err);

    void tls_emit_data(const uint8_t data[], size_t size) override;
    void tls_record_received(uint64_t seq_no,
                             const uint8_t data[],
                             size_t size) override;
    void tls_alert(Botan::TLS::Alert alert) override;
    bool tls_session_established(const Botan::TLS::Session &session) override;
    void tls_verify_cert_chain(
        const std::vector<Botan::X509_Certificate> &cert_chain,
        const std::vector<std::shared_ptr<const Botan::OCSP::Response>>
            &ocsp_responses,
        const std::vector<Botan::Certificate_Store *> &trusted_roots,
        Botan::Usage_Type usage,
        const std::string &hostname,
        const Botan::TLS::Policy &policy) override;
    std::string tls_server_choose_app_protocol(
        const std::vector<std::string> &client_protos) override;

    Botan::TLS::Default_Policy policy_;
    std::unique_ptr<Botan::Credentials_Manager> credsPtr_;
    std::unique_ptr<Botan::TLS::Channel> channel_;
    CertificatePtr peerCertPtr_;
    std::string sniName_;
    MsgBuffer recvBuffer_;
    // TODO: Rename this to avoid confusion
    SSLPolicyPtr policyPtr_;

    bool closingTLS_ = false;
    bool isServer_ = false;
};
}  // namespace trantor