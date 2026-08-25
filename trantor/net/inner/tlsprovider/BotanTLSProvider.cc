#include <trantor/net/inner/TLSProvider.h>
#include <trantor/net/Certificate.h>
#include <trantor/net/TcpConnection.h>
#include <trantor/net/callbacks.h>
#include <trantor/utils/Logger.h>

#include <botan/tls_server.h>
#include <botan/tls_client.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>
#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/certstor_system.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/pem.h>
#include <botan/tls_exceptn.h>
#include <botan/pkix_types.h>
#include <botan/certstor_flatfile.h>
#include <botan/x509path.h>
#include <botan/tls_session_manager_memory.h>
#include <memory>
#include <stdexcept>

using namespace trantor;
using namespace std::placeholders;

static std::once_flag sessionManagerInitFlag;
static std::shared_ptr<Botan::AutoSeeded_RNG> sessionManagerRng;
static std::shared_ptr<Botan::TLS::Session_Manager_In_Memory> sessionManager;
static thread_local std::shared_ptr<Botan::AutoSeeded_RNG> rng;

using namespace trantor;

static bool certificateMatchesHostname(const Botan::X509_Certificate &cert,
                                       std::string_view hostname)
{
// The string overload is the only matcher available across the supported
// Botan 3 releases, and unlike the newer typed overloads it also handles both
// DNS names and IP literals.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    const bool matches = cert.matches_dns_name(hostname);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    return matches;
}

static std::string join(const std::vector<std::string> &vec,
                        const std::string &delim)
{
    std::string ret;
    for (auto const &str : vec)
    {
        if (ret.empty() == false)
            ret += delim;
        ret += str;
    }
    return ret;
}

class Credentials : public Botan::Credentials_Manager
{
  public:
    Credentials(std::shared_ptr<Botan::Private_Key> key,
                const std::vector<Botan::X509_Certificate> *certChain,
                Botan::Certificate_Store *certStore)
        : certStore_(certStore), certChain_(certChain), key_(key)
    {
    }
    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        (void)type;
        (void)context;
        if (certStore_ == nullptr)
            return {};
        return {certStore_};
    }

    std::vector<Botan::X509_Certificate> find_cert_chain(
        const std::vector<std::string> &cert_key_types,
        const std::vector<Botan::AlgorithmIdentifier> &cert_signature_schemes,
        const std::vector<Botan::X509_DN> &acceptable_CAs,
        const std::string &type,
        const std::string &context) override
    {
        (void)type;
        (void)context;
        (void)cert_signature_schemes;
        (void)acceptable_CAs;
        if (certChain_ == nullptr || certChain_->empty())
            return {};

        auto key_algo = certChain_->front()
                            .subject_public_key_algo()
                            .oid()
                            .to_formatted_string();
        auto it =
            std::find(cert_key_types.begin(), cert_key_types.end(), key_algo);
        if (it == cert_key_types.end())
            return {};
        return *certChain_;
    }

    std::shared_ptr<Botan::Private_Key> private_key_for(
        const Botan::X509_Certificate &cert,
        const std::string &type,
        const std::string &context) override
    {
        (void)cert;
        (void)type;
        (void)context;
        return key_;
    }
    Botan::Certificate_Store *certStore_ = nullptr;
    const std::vector<Botan::X509_Certificate> *certChain_ = nullptr;
    std::shared_ptr<Botan::Private_Key> key_ = nullptr;
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

CertificatePtr trantor::Certificate::fromPem(const std::string &pem)
{
    try
    {
        Botan::DataSource_Memory source(pem);
        return std::make_shared<BotanCertificate>(
            Botan::X509_Certificate(source));
    }
    catch (const Botan::Exception &)
    {
        return nullptr;
    }
}

namespace trantor
{
struct SSLContext
{
    std::shared_ptr<Botan::Private_Key> key;
    std::vector<Botan::X509_Certificate> certChain;
    std::shared_ptr<Botan::Certificate_Store> certStore;
    bool isServer = false;
    bool requestClientCert = false;
    bool requireClientCert = false;
};
}  // namespace trantor

class TrantorPolicy : public Botan::TLS::Policy
{
    virtual bool require_cert_revocation_info() const override
    {
        return false;
    }

    virtual bool require_client_certificate_authentication() const override
    {
        return requireClientCert_;
    }

    virtual bool request_client_certificate_authentication() const override
    {
        return requestClientCert_;
    }

  public:
    bool requestClientCert_ = false;
    bool requireClientCert_ = false;
};

struct BotanTLSProvider : public TLSProvider,
                          public NonCopyable,
                          public Botan::TLS::Callbacks
{
  public:
    BotanTLSProvider(TcpConnection *conn,
                     TLSPolicyPtr policy,
                     SSLContextPtr ctx)
        : TLSProvider(conn, std::move(policy), std::move(ctx))
    {
        validationPolicy_ = std::make_shared<TrantorPolicy>();
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
            {
                if (e.type() == Botan::TLS::Alert::BadCertificate)
                    handleSSLError(SSLError::kSSLInvalidCertificate);
                else
                    handleSSLError(SSLError::kSSLHandshakeError);
            }
            else
                handleSSLError(SSLError::kSSLProtocolError);
        }
        catch (const Botan::Exception &e)
        {
            LOG_ERROR << "Unexpected Botan Exception: " << e.what();
            conn_->shutdown();
            if (tlsConnected_ == false)
                handleSSLError(SSLError::kSSLHandshakeError);
            else
                handleSSLError(SSLError::kSSLProtocolError);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Unexpected Generic Exception: " << e.what();
            conn_->shutdown();
            if (tlsConnected_ == false)
                handleSSLError(SSLError::kSSLHandshakeError);
            else
                handleSSLError(SSLError::kSSLProtocolError);
        }
        buffer->retrieveAll();
    }

    virtual ssize_t sendData(const char *ptr, size_t size) override
    {
        if (getBufferedData().readableBytes() != 0)
        {
            errno = EAGAIN;
            return 0;
        }

        // Limit the size of the data we send in one go to avoid holding massive
        // buffers in memory.
        constexpr size_t maxSend = 64 * 1024;
        size_t hasSent = 0;
        while (hasSent < size && getBufferedData().readableBytes() == 0)
        {
            auto trunkLen = size - hasSent;
            if (trunkLen > maxSend)
                trunkLen = maxSend;
            channel_->send((const uint8_t *)ptr + hasSent, trunkLen);
            // HACK: Botan doesn't provide a way to know how much raw data has
            // been written to the underlying transport. So we have to assume
            // that all data has been written. And cache the unwritten data in
            // writeBuffer_. Then "fake" the consumed size in sendData() to make
            // the caller think that all data has been written. Then return -1
            // if the underlying socket is not writable at all (i.e. write is
            // all or nothing)
            if (lastWriteSize_ == -1)
                return -1;
            hasSent += trunkLen;
        }
        return static_cast<ssize_t>(hasSent);
    }

    virtual void close() override
    {
        if (channel_ && channel_->is_active())
        {
            channel_->close();
        }
    }

    virtual void startEncryption() override
    {
        auto certStorePtr = contextPtr_->certStore.get();
        credsPtr_ = std::make_shared<Credentials>(contextPtr_->key,
                                                  &contextPtr_->certChain,
                                                  certStorePtr);
        if (policyPtr_->getConfCmds().empty() == false)
            LOG_WARN << "BotanTLSConnectionImpl does not support sslConfCmds.";

        // initialize rng and session manager if we haven't already
        std::call_once(sessionManagerInitFlag, []() {
            sessionManagerRng = std::make_shared<Botan::AutoSeeded_RNG>();
            sessionManager =
                std::make_shared<Botan::TLS::Session_Manager_In_Memory>(
                    sessionManagerRng);
        });
        if (rng == nullptr)
            rng = std::make_shared<Botan::AutoSeeded_RNG>();

        auto fakeThis = std::shared_ptr<BotanTLSProvider>(this, [](auto) {});
        if (contextPtr_->isServer)
        {
            // TODO: Need a more scalable way to manage session validation rules
            validationPolicy_->requireClientCert_ =
                contextPtr_->requireClientCert;
            validationPolicy_->requestClientCert_ =
                contextPtr_->requestClientCert;
            channel_ = std::make_unique<Botan::TLS::Server>(std::move(fakeThis),
                                                            sessionManager,
                                                            credsPtr_,
                                                            validationPolicy_,
                                                            rng);
        }
        else
        {
            validationPolicy_->requireClientCert_ =
                contextPtr_->requireClientCert;
            validationPolicy_->requestClientCert_ =
                contextPtr_->requestClientCert;
            // technically Botan2 does support TLS 1.0 and 1.1, but Botan3 does
            // not. So we just disable them to keep compatibility.
            if (policyPtr_->getUseOldTLS())
                LOG_WARN << "Old TLS not supported by Botan (only >= TLS 1.2)";
            channel_ = std::make_unique<Botan::TLS::Client>(
                std::move(fakeThis),
                sessionManager,
                credsPtr_,
                validationPolicy_,
                rng,
                Botan::TLS::Server_Information(policyPtr_->getHostname(),
                                               conn_->peerAddr().toPort()),
                Botan::TLS::Protocol_Version::TLS_V12,
                policyPtr_->getAlpnProtocols());
            setSniName(policyPtr_->getHostname());
        }
    }

    void handleSSLError(SSLError err)
    {
        if (!errorCallback_)
            return;
        loop_->queueInLoop([this, err]() { errorCallback_(conn_, err); });
    }

    virtual ~BotanTLSProvider() override = default;

    void tls_emit_data(std::span<const uint8_t> data) override
    {
        if (getBufferedData().readableBytes() != 0)
        {
            appendToWriteBuffer((const char *)data.data(), data.size_bytes());
            return;
        }

        auto n = writeCallback_(conn_, data.data(), data.size_bytes());
        lastWriteSize_ = n;

        // store the unsent data and send it later
        if (n == ssize_t(data.size_bytes()))
            return;
        if (n == -1)
            n = 0;
        appendToWriteBuffer((const char *)data.data() + n,
                            data.size_bytes() - n);
    }

    void tls_record_received(uint64_t seq_no,
                             std::span<const uint8_t> data) override
    {
        (void)seq_no;
        recvBuffer_.append((const char *)data.data(), data.size_bytes());
        if (messageCallback_)
            messageCallback_(conn_, &recvBuffer_);
    }

    std::string tls_server_choose_app_protocol(
        const std::vector<std::string> &client_protos) override
    {
        assert(contextPtr_->isServer);
        if (policyPtr_->getAlpnProtocols().empty() || client_protos.empty())
            return "";

        for (auto const &proto : client_protos)
        {
            if (std::find(policyPtr_->getAlpnProtocols().begin(),
                          policyPtr_->getAlpnProtocols().end(),
                          proto) != policyPtr_->getAlpnProtocols().end())
                return proto;
        }

        throw Botan::TLS::TLS_Exception(
            Botan::TLS::Alert::NoApplicationProtocol,
            "No supported application protocol found. Client offered: " +
                join(client_protos, ", ") + " but we support: " +
                join(policyPtr_->getAlpnProtocols(), ", "));
    }

    void tls_alert(Botan::TLS::Alert alert) override
    {
        if (alert.type() == Botan::TLS::Alert::CloseNotify)
        {
            LOG_TRACE << "TLS close notify received";
            if (closeCallback_)
                closeCallback_(conn_);
        }
        else
        {
            if (errorCallback_)
                errorCallback_(conn_, SSLError::kSSLProtocolError);
        }
    }

    void tls_session_activated() override
    {
        LOG_TRACE << "tls_session_activated";
        tlsConnected_ = true;
        setApplicationProtocol(channel_->application_protocol());
        if (contextPtr_->isServer && !contextPtr_->certChain.empty())
            setLocalCertificate(std::make_shared<BotanCertificate>(
                contextPtr_->certChain.front()));
        if (handshakeCallback_)
            handshakeCallback_(conn_);
    }

    void tls_verify_cert_chain(
        const std::vector<Botan::X509_Certificate> &certs,
        const std::vector<std::optional<Botan::OCSP::Response>> &ocsp,
        const std::vector<Botan::Certificate_Store *> &trusted_roots,
        Botan::Usage_Type usage,
        std::string_view hostname,
        const Botan::TLS::Policy &policy) override
    {
        setSniName(std::string(hostname));
        if (policyPtr_->getValidate())
        {
            if (certs.size() == 0)
                throw Botan::TLS::TLS_Exception(
                    Botan::TLS::Alert::NoCertificate,
                    "Certificate validation failed: no certificate");
            if (policyPtr_->getAllowBrokenChain())
            {
                const auto &cert = certs[0];
                const auto now = std::chrono::system_clock::now();
                if (now < cert.not_before().to_std_timepoint() ||
                    now > cert.not_after().to_std_timepoint())
                {
                    throw Botan::TLS::TLS_Exception(
                        Botan::TLS::Alert::CertificateExpired,
                        "Certificate validation failed: certificate is not "
                        "currently valid");
                }
                if (!contextPtr_->isServer &&
                    !certificateMatchesHostname(cert, hostname))
                {
                    throw Botan::TLS::TLS_Exception(
                        Botan::TLS::Alert::BadCertificate,
                        "Certificate validation failed: hostname mismatch");
                }
            }
            else
            {
                Botan::TLS::Callbacks::tls_verify_cert_chain(
                    certs, ocsp, trusted_roots, usage, hostname, policy);
            }
        }

        if (certs.size() > 0)
            setPeerCertificate(std::make_shared<BotanCertificate>(certs[0]));
    }

    std::shared_ptr<TrantorPolicy> validationPolicy_;
    std::shared_ptr<Botan::Credentials_Manager> credsPtr_;
    std::unique_ptr<Botan::TLS::Channel> channel_;
    bool tlsConnected_ = false;
    ssize_t lastWriteSize_ = 0;
};

std::shared_ptr<TLSProvider> trantor::newTLSProvider(TcpConnection *conn,
                                                     TLSPolicyPtr policy,
                                                     SSLContextPtr ctx)
{
    return std::make_shared<BotanTLSProvider>(conn,
                                              std::move(policy),
                                              std::move(ctx));
}

SSLContextPtr trantor::newSSLContext(const TLSPolicy &policy, bool server)
{
    if (server && policy.getServerCertificateProvider())
    {
        throw std::runtime_error(
            "TLSPolicy::setServerCertificateProvider is not supported by "
            "the Botan TLS provider");
    }

    auto ctx = std::make_shared<SSLContext>();
    ctx->isServer = server;

    auto loadCertificateChain = [](Botan::DataSource &source) {
        std::vector<Botan::X509_Certificate> chain;
        while (!source.end_of_data())
        {
            std::string label;
            auto certBits = Botan::PEM_Code::decode(source, label);
            if (label == "CERTIFICATE" || label == "TRUSTED CERTIFICATE")
                chain.emplace_back(certBits);
        }
        if (chain.empty())
            throw std::runtime_error("Failed to load certificate PEM");
        return chain;
    };

    if (!policy.getCertificatePem().empty())
    {
        Botan::DataSource_Memory keySource(
            policy.getPrivateKeyPem().empty() ? policy.getCertificatePem() :
                                                policy.getPrivateKeyPem());
        ctx->key = Botan::PKCS8::load_key(keySource);
        Botan::DataSource_Memory certSource(policy.getCertificatePem());
        ctx->certChain = loadCertificateChain(certSource);
    }
    else if (!policy.getKeyPath().empty())
    {
        Botan::DataSource_Stream in(policy.getKeyPath());
        ctx->key = Botan::PKCS8::load_key(in);
    }

    if (!policy.getCertPath().empty())
    {
        Botan::DataSource_Stream certSource(policy.getCertPath());
        ctx->certChain = loadCertificateChain(certSource);
    }

    if (policy.getValidate())
    {
        if (!policy.getCaPath().empty())
        {
            ctx->certStore =
                std::make_shared<Botan::Flatfile_Certificate_Store>(
                    policy.getCaPath());
        }
        else if (policy.getUseSystemCertStore())
        {
            static auto systemCertStore =
                std::make_shared<Botan::System_Certificate_Store>();
            ctx->certStore = systemCertStore;
        }
    }
    if (server && policy.getPeerCertificateRequest())
    {
        ctx->requestClientCert = true;
        ctx->requireClientCert = policy.getRequirePeerCertificate();
    }
    else if (server && policy.getValidate() && !policy.getCaPath().empty())
    {
        // Preserve the legacy CA-path behaviour for callers that do not use
        // the explicit request API.
        ctx->requestClientCert = true;
        ctx->requireClientCert = true;
    }

    if (policy.getUseOldTLS())
        LOG_WARN << "SSLPloicy have set useOldTLS to true. BUt Botan does not "
                    "support TLS/SSL below TLS 1.2. Ignoring this option.";
    return ctx;
}
