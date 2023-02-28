#include <trantor/net/inner/TLSProvider.h>
#include <trantor/net/Certificate.h>
#include <trantor/net/TcpConnection.h>
#include <trantor/net/callbacks.h>
#include <trantor/utils/Logger.h>

#include <fstream>

#include <botan/tls_server.h>
#include <botan/tls_client.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>
#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/certstor_system.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/tls_exceptn.h>
#include <botan/pkix_types.h>
#include <botan/certstor_flatfile.h>

using namespace trantor;
using namespace std::placeholders;

static Botan::System_RNG sessionManagerRng;
// Is this Ok? C++ technically doesn't guarantee static object initialization
// order.
static Botan::TLS::Session_Manager_In_Memory sessionManager(sessionManagerRng);
static thread_local Botan::System_RNG rng;
static thread_local Botan::System_Certificate_Store certStore;

using namespace trantor;

class Credentials : public Botan::Credentials_Manager
{
  public:
    Credentials(Botan::Private_Key *key,
                Botan::X509_Certificate *cert,
                Botan::Certificate_Store *certStore)
        : key_(key), cert_(cert), certStore_(certStore)
    {
    }
    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        if (certStore_ == nullptr)
            return {};
        return {certStore_};
    }

    std::vector<Botan::X509_Certificate> cert_chain(
        const std::vector<std::string> &cert_key_types,
        const std::string &type,
        const std::string &context) override
    {
        if (cert_ == nullptr)
            return {};

        auto key_algo =
            cert_->subject_public_key_algo().get_oid().to_formatted_string();
        auto it =
            std::find(cert_key_types.begin(), cert_key_types.end(), key_algo);
        if (it == cert_key_types.end())
            return {};
        return {*cert_};
    }

    Botan::Private_Key *private_key_for(const Botan::X509_Certificate &cert,
                                        const std::string &type,
                                        const std::string &context) override
    {
        return key_;
    }
    Botan::Certificate_Store *certStore_ = nullptr;
    Botan::Private_Key *key_ = nullptr;
    Botan::X509_Certificate *cert_ = nullptr;
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

namespace trantor
{
struct SSLContext
{
    std::unique_ptr<Botan::Private_Key> key;
    std::unique_ptr<Botan::X509_Certificate> cert;
    std::unique_ptr<Botan::Certificate_Store> certStore;
    bool isServer = false;
};
}  // namespace trantor

class TrantorPolicy : public Botan::TLS::Policy
{
    virtual bool require_cert_revocation_info() const override
    {
        return false;
    }
};

struct BotanTLSProvider : public TLSProvider,
                          public NonCopyable,
                          public Botan::TLS::Callbacks
{
  public:
    BotanTLSProvider(EventLoop *loop,
                     TcpConnection *conn,
                     SSLPolicyPtr policy,
                     SSLContextPtr ctx)
        : TLSProvider(loop, conn, policy), policyPtr_(policy), contextPtr_(ctx)
    {
    }

    virtual void recvData(MsgBuffer *buffer) override
    {
        LOG_TRACE << "Low level connection received " << buffer->readableBytes()
                  << " bytes.";
        try
        {
            assert(channel_ != nullptr);
            channel_->received_data((const uint8_t *)buffer->peek(),
                                    buffer->readableBytes());
        }
        catch (const Botan::TLS::TLS_Exception &e)
        {
            LOG_ERROR << "Unexpected TLS Exception: " << e.what();
            conn_->shutdown();

            if (tlsConnected_ == false)
                handleSSLError(SSLError::kSSLHandshakeError);
            else
                handleSSLError(SSLError::kSSLProtocolError);
        }
        buffer->retrieveAll();
    }

    virtual void sendData(const char *ptr, size_t size) override
    {
        channel_->send((const uint8_t *)ptr, size);
    }

    virtual void startEncryption() override
    {
        LOG_DEBUG << "certStore " << contextPtr_->certStore.get();
        credsPtr_ = std::make_unique<Credentials>(contextPtr_->key.get(),
                                                  contextPtr_->cert.get(),
                                                  contextPtr_->certStore.get());
        if (policyPtr_->getConfCmds().empty() == false)
            LOG_WARN << "BotanTLSConnectionImpl does not support sslConfCmds.";
        if (contextPtr_->isServer)
        {
            channel_ = std::make_unique<Botan::TLS::Server>(
                *this, sessionManager, *credsPtr_, validationPolicy_, rng);
        }
        else
        {
            channel_ = std::make_unique<Botan::TLS::Client>(
                *this,
                sessionManager,
                *credsPtr_,
                validationPolicy_,
                rng,
                Botan::TLS::Server_Information(policyPtr_->getHostname()),
                policyPtr_->getUseOldTLS()
                    ? Botan::TLS::Protocol_Version::TLS_V10
                    : Botan::TLS::Protocol_Version::TLS_V12,
                policyPtr_->getAlpnProtocols());
        }
    }

    void handleSSLError(SSLError err)
    {
        if (!errorCallback_)
            return;
        loop_->queueInLoop([this]() {
            errorCallback_(conn_, SSLError::kSSLInvalidCertificate);
        });
    }

    virtual ~BotanTLSProvider() override = default;

    void tls_emit_data(const uint8_t data[], size_t size) override
    {
        MsgBuffer buf;
        buf.append((const char *)data, size);
        assert(writeCallback_);
        writeCallback_(conn_, buf);
    }

    void tls_record_received(uint64_t seq_no,
                             const uint8_t data[],
                             size_t size) override
    {
        recvBuffer_.append((const char *)data, size);
        if (messageCallback_)
            messageCallback_(conn_, &recvBuffer_);
    }

    void tls_alert(Botan::TLS::Alert alert) override
    {
        if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
        {
            if (closeCallback_)
                closeCallback_(conn_);
        }
        else
        {
            if (errorCallback_)
                errorCallback_(conn_, SSLError::kSSLProtocolError);
        }
    }

    bool tls_session_established(const Botan::TLS::Session &session)
    {
        LOG_TRACE << "tls_session_established";
        bool needCerts =
            policyPtr_->getValidateDate() || policyPtr_->getValidateDomain();
        const auto &certs = session.peer_certs();
        if (needCerts && session.peer_certs().empty())
        {
            handleSSLError(SSLError::kSSLInvalidCertificate);
            throw std::runtime_error("No certificates provided by peer");
        }
        if (!certs.empty())
        {
            auto &cert = certs[0];
            if (policyPtr_->getValidateDomain() &&
                cert.matches_dns_name(policyPtr_->getHostname()) == false)
            {
                handleSSLError(SSLError::kSSLInvalidCertificate);
                throw std::runtime_error("Certificate does not match hostname");
            }
            if (policyPtr_->getValidateDate())
            {
                auto notBefore = cert.not_before();
                auto notAfter = cert.not_after();
                auto now = Botan::ASN1_Time(std::chrono::system_clock::now());
                if (now < notBefore || now > notAfter)
                {
                    handleSSLError(SSLError::kSSLInvalidCertificate);
                    throw std::runtime_error(
                        "Certificate is not valid for current time");
                }
            }
            peerCertPtr_ = std::make_shared<BotanCertificate>(cert);
        }

        tlsConnected_ = true;
        loop_->queueInLoop([this]() {
            if (policyPtr_->getOneShotConnctionCallback() && !oneshotCalled_)
            {
                oneshotCalled_ = true;
                abort();
                // policyPtr_->getOneShotConnctionCallback()(shared_from_this());
            }
            else if (handshakeCallback_)
                handshakeCallback_(conn_);
        });
        // Do we want to cache all sessions?
        return true;
    }

    void tls_verify_cert_chain(
        const std::vector<Botan::X509_Certificate> &certs,
        const std::vector<std::shared_ptr<const Botan::OCSP::Response>> &ocsp,
        const std::vector<Botan::Certificate_Store *> &trusted_roots,
        Botan::Usage_Type usage,
        const std::string &hostname,
        const Botan::TLS::Policy &policy)
    {
        sniName_ = hostname;
        if (policyPtr_->getValidateChain())
            Botan::TLS::Callbacks::tls_verify_cert_chain(
                certs, ocsp, trusted_roots, usage, hostname, policy);
    }

    TrantorPolicy validationPolicy_;
    std::unique_ptr<Botan::Credentials_Manager> credsPtr_;
    std::unique_ptr<Botan::TLS::Channel> channel_;
    CertificatePtr peerCertPtr_;
    std::string sniName_;
    MsgBuffer recvBuffer_;
    const SSLPolicyPtr policyPtr_;
    const SSLContextPtr contextPtr_;
    bool oneshotCalled_ = false;
    bool tlsConnected_ = false;
};

std::unique_ptr<TLSProvider> trantor::newTLSProvider(EventLoop *loop,
                                                     TcpConnection *conn,
                                                     SSLPolicyPtr policy,
                                                     SSLContextPtr ctx)
{
    return std::make_unique<BotanTLSProvider>(loop,
                                              conn,
                                              std::move(policy),
                                              std::move(ctx));
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy, bool server)
{
    auto ctx = std::make_shared<SSLContext>();
    ctx->isServer = server;
    if (!policy.getKeyPath().empty())
    {
        Botan::DataSource_Stream in(policy.getKeyPath());
        ctx->key = Botan::PKCS8::load_key(in);
    }

    if (!policy.getCertPath().empty())
    {
        ctx->cert =
            std::make_unique<Botan::X509_Certificate>(policy.getCertPath());
    }

    if (policy.getValidateChain() && !policy.getCaPath().empty())
    {
        ctx->certStore = std::make_unique<Botan::Flatfile_Certificate_Store>(
            policy.getCaPath());
    }
    else if (policy.getValidateChain() && policy.getUseSystemCertStore())
    {
        ctx->certStore = std::make_unique<Botan::System_Certificate_Store>();
    }

    if (policy.getUseOldTLS())
        LOG_WARN << "SSLPloicy have set useOldTLS to true. BUt Botan does not "
                    "support TLS/SSL below TLS 1.2. Ignoreing this option.";
    return ctx;
}
