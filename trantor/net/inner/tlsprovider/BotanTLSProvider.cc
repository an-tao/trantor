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
#include <botan/tls_session.h>
#include <botan/pkix_types.h>
#include <botan/certstor_flatfile.h>
#include <botan/x509path.h>
#include <botan/tls_session_manager_memory.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace trantor;

static thread_local auto threadRng = std::make_shared<Botan::AutoSeeded_RNG>();

static bool isCertificateAlert(Botan::TLS::Alert::Type type)
{
    switch (type)
    {
        case Botan::TLS::Alert::NoCertificate:
        case Botan::TLS::Alert::BadCertificate:
        case Botan::TLS::Alert::UnsupportedCertificate:
        case Botan::TLS::Alert::CertificateRevoked:
        case Botan::TLS::Alert::CertificateExpired:
        case Botan::TLS::Alert::CertificateUnknown:
        case Botan::TLS::Alert::UnknownCA:
        case Botan::TLS::Alert::CertificateUnobtainable:
        case Botan::TLS::Alert::BadCertificateStatusResponse:
        case Botan::TLS::Alert::BadCertificateHashValue:
        case Botan::TLS::Alert::CertificateRequired:
            return true;
        default:
            return false;
    }
}

static SSLError sslErrorForAlert(Botan::TLS::Alert::Type type,
                                 bool tlsConnected)
{
    if (isCertificateAlert(type))
        return SSLError::kSSLInvalidCertificate;
    return tlsConnected ? SSLError::kSSLProtocolError
                        : SSLError::kSSLHandshakeError;
}

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

static std::vector<Botan::X509_Certificate> loadCertificateChain(
    Botan::DataSource &source)
{
    std::vector<Botan::X509_Certificate> chain;
    while (!source.end_of_data())
    {
        std::string label;
        Botan::secure_vector<uint8_t> certBits;
        try
        {
            certBits = Botan::PEM_Code::decode(source, label);
        }
        catch (const Botan::Decoding_Error &)
        {
            break;
        }
        if (label == "CERTIFICATE" || label == "TRUSTED CERTIFICATE")
            chain.emplace_back(certBits);
    }
    if (chain.empty())
        throw std::runtime_error("Failed to load certificate PEM");
    return chain;
}

static bool certificateMatchesPrivateKey(
    const Botan::X509_Certificate &certificate,
    const Botan::Private_Key &privateKey)
{
    return certificate.subject_public_key_info() ==
           privateKey.subject_public_key();
}

static void validateCertificateAndKey(
    const std::vector<Botan::X509_Certificate> &certChain,
    const Botan::Private_Key *key)
{
    if (!certChain.empty() && key != nullptr &&
        !certificateMatchesPrivateKey(certChain.front(), *key))
        throw std::runtime_error(
            "The private key does not match the leaf certificate");
}

static bool certificateChainMatchesSignatureSchemes(
    const std::vector<Botan::X509_Certificate> &certChain,
    const std::vector<Botan::AlgorithmIdentifier> &signatureSchemes)
{
    if (signatureSchemes.empty())
        return true;

    return std::all_of(
        certChain.begin(), certChain.end(), [&signatureSchemes](const auto &cert) {
            return std::find(signatureSchemes.begin(),
                             signatureSchemes.end(),
                             cert.signature_algorithm()) !=
                   signatureSchemes.end();
        });
}

static bool certificateChainMatchesAuthorities(
    const std::vector<Botan::X509_Certificate> &certChain,
    const std::vector<Botan::X509_DN> &acceptableCAs)
{
    if (acceptableCAs.empty())
        return true;

    return std::any_of(
        certChain.begin(), certChain.end(), [&acceptableCAs](const auto &cert) {
            return std::find(acceptableCAs.begin(),
                             acceptableCAs.end(),
                             cert.issuer_dn()) != acceptableCAs.end();
        });
}

class InputBufferDrainer
{
  public:
    explicit InputBufferDrainer(MsgBuffer *buffer) : buffer_(buffer)
    {
    }

    ~InputBufferDrainer()
    {
        drain();
    }

    void drain()
    {
        if (buffer_ != nullptr)
        {
            buffer_->retrieveAll();
            buffer_ = nullptr;
        }
    }

  private:
    MsgBuffer *buffer_;
};

class Credentials : public Botan::Credentials_Manager
{
  private:
    enum class SelectionSource
    {
        Unselected,
        Configured,
        Provider
    };

    struct SelectedCredentials
    {
        std::shared_ptr<Botan::Private_Key> key;
        std::vector<Botan::X509_Certificate> certChain;
        std::string serverName;
        SelectionSource source = SelectionSource::Unselected;
        bool certificateChainSelected = false;
    };

  public:
    Credentials(std::shared_ptr<Botan::Private_Key> key,
                std::vector<Botan::X509_Certificate> certChain,
                std::shared_ptr<Botan::Certificate_Store> certStore,
                ServerCertificateProvider certificateProvider)
        : configuredKey_(std::move(key)),
          configuredCertChain_(std::move(certChain)),
          certStore_(std::move(certStore)),
          certificateProvider_(std::move(certificateProvider))
    {
        resetHandshakeSelection();
    }

    std::vector<Botan::Certificate_Store *> trusted_certificate_authorities(
        const std::string &type,
        const std::string &context) override
    {
        (void)type;
        (void)context;
        if (!certStore_)
            return {};
        return {certStore_.get()};
    }

    std::vector<Botan::X509_Certificate> find_cert_chain(
        const std::vector<std::string> &cert_key_types,
        const std::vector<Botan::AlgorithmIdentifier> &cert_signature_schemes,
        const std::vector<Botan::X509_DN> &acceptable_CAs,
        const std::string &type,
        const std::string &context) override
    {
        const auto &credentials = activeCredentials(type, context);
        if (credentials.certChain.empty())
            return {};

        const auto key_algo = credentials.certChain.front()
                                  .subject_public_key_algo()
                                  .oid()
                                  .to_formatted_string();
        const auto it =
            std::find(cert_key_types.begin(), cert_key_types.end(), key_algo);
        if (!cert_key_types.empty() && it == cert_key_types.end())
            return {};
        if (!certificateChainMatchesSignatureSchemes(
                credentials.certChain, cert_signature_schemes))
            return {};
        if (!certificateChainMatchesAuthorities(credentials.certChain,
                                                acceptable_CAs))
            return {};
        selectedCredentials_.certificateChainSelected = true;
        return credentials.certChain;
    }

    std::shared_ptr<Botan::Private_Key> private_key_for(
        const Botan::X509_Certificate &cert,
        const std::string &type,
        const std::string &context) override
    {
        (void)type;
        (void)context;
        const auto &credentials = activeCredentials();
        if (credentials.certChain.empty() ||
            cert != credentials.certChain.front())
            return nullptr;
        return credentials.key;
    }

    bool ensure_server_credentials(const std::string &serverName)
    {
        if (selectedCredentials_.source == SelectionSource::Unselected)
            activeCredentials("tls-server", serverName);
        return selectedCredentials_.key != nullptr &&
               !selectedCredentials_.certChain.empty();
    }

    const Botan::X509_Certificate *selected_leaf_certificate() const
    {
        const auto &credentials = activeCredentials();
        if (credentials.certChain.empty())
            return nullptr;
        return &credentials.certChain.front();
    }

    const std::string &selected_server_name() const
    {
        return activeCredentials().serverName;
    }

    const Botan::X509_Certificate *selected_client_leaf_certificate() const
    {
        const auto &credentials = activeCredentials();
        if (!credentials.certificateChainSelected ||
            credentials.certChain.empty())
            return nullptr;
        return &credentials.certChain.front();
    }

    void finish_handshake()
    {
        resetHandshakeSelection();
    }

  private:
    void resetHandshakeSelection()
    {
        selectedCredentials_ = {};
        selectedCredentials_.key = configuredKey_;
        selectedCredentials_.certChain = configuredCertChain_;
    }

    const SelectedCredentials &activeCredentials(const std::string &type,
                                                 const std::string &context)
    {
        if (type != "tls-server")
            return selectedCredentials_;

        if (!certificateProvider_)
        {
            selectedCredentials_.serverName = context;
            selectedCredentials_.source = SelectionSource::Configured;
            return selectedCredentials_;
        }

        if (selectedCredentials_.source == SelectionSource::Provider &&
            selectedCredentials_.serverName == context)
            return selectedCredentials_;

        // Each Credentials instance belongs to one TLS connection. Remember
        // both successful and failed provider results so Botan can repeat its
        // lookup during a handshake without calling application code again.
        selectedCredentials_ = {};
        selectedCredentials_.serverName = context;
        selectedCredentials_.source = SelectionSource::Provider;
        try
        {
            const auto certificate = certificateProvider_(context);
            if (certificate.certificatePem.empty() ||
                certificate.privateKeyPem.empty())
                return selectedCredentials_;

            Botan::DataSource_Memory keySource(certificate.privateKeyPem);
            auto key = Botan::PKCS8::load_key(keySource);
            Botan::DataSource_Memory certSource(certificate.certificatePem);
            auto certChain = loadCertificateChain(certSource);
            validateCertificateAndKey(certChain, key.get());
            selectedCredentials_.key = std::move(key);
            selectedCredentials_.certChain = std::move(certChain);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Server certificate provider failed: " << e.what();
        }
        return selectedCredentials_;
    }

    const SelectedCredentials &activeCredentials() const
    {
        return selectedCredentials_;
    }

    std::shared_ptr<Botan::Private_Key> configuredKey_;
    std::vector<Botan::X509_Certificate> configuredCertChain_;
    std::shared_ptr<Botan::Certificate_Store> certStore_;
    ServerCertificateProvider certificateProvider_;
    SelectedCredentials selectedCredentials_;
};

struct BotanCertificate : public Certificate
{
    BotanCertificate(const Botan::X509_Certificate &cert) : cert_(cert)
    {
    }

    std::string sha1Fingerprint() const override
    {
        return cert_.fingerprint("SHA-1");
    }

    std::string sha256Fingerprint() const override
    {
        return cert_.fingerprint("SHA-256");
    }

    std::string pem() const override
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
    std::shared_ptr<Botan::TLS::Session_Manager_In_Memory> sessionManager;
    ServerCertificateProvider certificateProvider;
    std::vector<std::string> alpnProtocols;
    std::string hostname;
    bool isServer = false;
    bool validate = true;
    bool allowBrokenChain = false;
    bool requestClientCert = false;
    bool requireClientCert = false;
};
}  // namespace trantor

class TrantorPolicy : public Botan::TLS::Policy
{
  public:
    TrantorPolicy(bool requestClientCert, bool requireClientCert)
        : requestClientCert_(requestClientCert),
          requireClientCert_(requireClientCert)
    {
    }

  private:
    bool require_cert_revocation_info() const override
    {
        return false;
    }

    bool require_client_certificate_authentication() const override
    {
        return requireClientCert_;
    }

    bool request_client_certificate_authentication() const override
    {
        return requestClientCert_;
    }

    const bool requestClientCert_;
    const bool requireClientCert_;
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
        validationPolicy_ =
            std::make_shared<TrantorPolicy>(contextPtr_->requestClientCert,
                                            contextPtr_->requireClientCert);
    }

    void recvData(MsgBuffer *buffer) override
    {
        LOG_TRACE << "Low level connection received " << buffer->readableBytes()
                  << " bytes.";
        InputBufferDrainer inputDrainer(buffer);
        bool receivedSuccessfully = false;
        try
        {
            assert(channel_ != nullptr);
            channel_->received_data(reinterpret_cast<const uint8_t *>(
                                        buffer->peek()),
                                    buffer->readableBytes());
            receivedSuccessfully = true;
        }
        catch (const Botan::TLS::TLS_Exception &e)
        {
            handleBotanException("receiving TLS data", e);
        }
        catch (const Botan::Exception &e)
        {
            handleBotanException("receiving TLS data", e);
        }
        catch (...)
        {
            messageCallbackPending_ = false;
            throw;
        }
        inputDrainer.drain();
        if (!receivedSuccessfully)
        {
            messageCallbackPending_ = false;
            return;
        }
        if (dispatchPendingAlert())
        {
            messageCallbackPending_ = false;
            return;
        }
        if (messageCallbackPending_)
        {
            messageCallbackPending_ = false;
            if (messageCallback_)
                messageCallback_(conn_, &recvBuffer_);
        }
    }

    ssize_t sendData(const char *ptr, size_t size) override
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
            const auto chunkLen = (std::min)(size - hasSent, maxSend);
            try
            {
                channel_->send(
                    reinterpret_cast<const uint8_t *>(ptr) + hasSent,
                    chunkLen);
                if (dispatchPendingAlert())
                    return -1;
            }
            catch (const Botan::TLS::TLS_Exception &e)
            {
                handleBotanException("sending TLS data", e);
                return -1;
            }
            catch (const Botan::Exception &e)
            {
                handleBotanException("sending TLS data", e);
                return -1;
            }
            // HACK: Botan doesn't provide a way to know how much raw data has
            // been written to the underlying transport. So we have to assume
            // that all data has been written. And cache the unwritten data in
            // writeBuffer_. Then "fake" the consumed size in sendData() to make
            // the caller think that all data has been written. Then return -1
            // if the underlying socket is not writable at all (i.e. write is
            // all or nothing)
            if (lastWriteSize_ == -1)
                return -1;
            hasSent += chunkLen;
        }
        return static_cast<ssize_t>(hasSent);
    }

    void close() override
    {
        if (channel_ && channel_->is_active())
        {
            try
            {
                channel_->close();
                dispatchPendingAlert();
            }
            catch (const Botan::TLS::TLS_Exception &e)
            {
                handleBotanException("closing the TLS channel", e);
            }
            catch (const Botan::Exception &e)
            {
                handleBotanException("closing the TLS channel", e);
            }
        }
    }

    void startEncryption() override
    {
        credsPtr_ = std::make_shared<Credentials>(
            contextPtr_->key,
            contextPtr_->certChain,
            contextPtr_->certStore,
            contextPtr_->certificateProvider);

        // channel_ is a strict child of this provider, so its callbacks cannot
        // outlive us. A genuinely owning shared_ptr would create a cycle:
        // provider -> channel -> callbacks -> provider.
        auto callbacks = std::shared_ptr<Botan::TLS::Callbacks>(
            this, [](Botan::TLS::Callbacks *) {});
        try
        {
            if (contextPtr_->isServer)
            {
                channel_ = std::make_unique<Botan::TLS::Server>(
                    std::move(callbacks),
                    contextPtr_->sessionManager,
                    credsPtr_,
                    validationPolicy_,
                    threadRng);
            }
            else
            {
                channel_ = std::make_unique<Botan::TLS::Client>(
                    std::move(callbacks),
                    contextPtr_->sessionManager,
                    credsPtr_,
                    validationPolicy_,
                    threadRng,
                    Botan::TLS::Server_Information(
                        contextPtr_->hostname, conn_->peerAddr().toPort()),
                    Botan::TLS::Protocol_Version::latest_tls_version(),
                    contextPtr_->alpnProtocols);
                setSniName(contextPtr_->hostname);
            }
            dispatchPendingAlert();
        }
        catch (const Botan::TLS::TLS_Exception &e)
        {
            handleBotanException("starting TLS", e);
        }
        catch (const Botan::Exception &e)
        {
            handleBotanException("starting TLS", e);
        }
    }

    void handleBotanException(
        const char *operation,
        const Botan::TLS::TLS_Exception &exception)
    {
        handleBotanException(
            operation,
            exception,
            sslErrorForAlert(exception.type(), tlsConnected_));
    }

    void handleBotanException(const char *operation,
                              const Botan::Exception &exception)
    {
        handleBotanException(operation,
                             exception,
                             tlsConnected_ ? SSLError::kSSLProtocolError
                                           : SSLError::kSSLHandshakeError);
    }

    void handleBotanException(const char *operation,
                              const Botan::Exception &exception,
                              SSLError error)
    {
        pendingAlert_.reset();
        LOG_ERROR << "Botan failed while " << operation << ": "
                  << exception.what();
        handleSSLError(error);
    }

    void handleSSLError(SSLError err)
    {
        if (processedSslError_)
            return;
        processedSslError_ = true;
        if (!errorCallback_)
            return;

        // recvData() and startEncryption() run on the connection's loop. Keep
        // the provider alive across the callback because the callback closes
        // the owning connection synchronously.
        auto guard = shared_from_this();
        errorCallback_(conn_, err);
    }

    ~BotanTLSProvider() override = default;

    void tls_emit_data(std::span<const uint8_t> data) override
    {
        if (getBufferedData().readableBytes() != 0)
        {
            appendToWriteBuffer(reinterpret_cast<const char *>(data.data()),
                                data.size_bytes());
            return;
        }

        auto n = writeCallback_(conn_, data.data(), data.size_bytes());
        lastWriteSize_ = n;

        // store the unsent data and send it later
        if (n == static_cast<ssize_t>(data.size_bytes()))
            return;
        if (n == -1)
            n = 0;
        appendToWriteBuffer(reinterpret_cast<const char *>(data.data()) + n,
                            data.size_bytes() - n);
    }

    void tls_record_received(uint64_t seq_no,
                             std::span<const uint8_t> data) override
    {
        (void)seq_no;
        recvBuffer_.append(reinterpret_cast<const char *>(data.data()),
                           data.size_bytes());
        messageCallbackPending_ = true;
    }

    std::string tls_server_choose_app_protocol(
        const std::vector<std::string> &client_protos) override
    {
        assert(contextPtr_->isServer);
        if (contextPtr_->alpnProtocols.empty() || client_protos.empty())
            return "";

        for (const auto &proto : contextPtr_->alpnProtocols)
        {
            if (std::find(client_protos.begin(), client_protos.end(), proto) !=
                client_protos.end())
                return proto;
        }

        return "";
    }

    void tls_alert(Botan::TLS::Alert alert) override
    {
        if (alert.type() == Botan::TLS::Alert::CloseNotify)
        {
            if (!pendingAlert_ || !pendingAlert_->is_fatal())
                pendingAlert_ = std::move(alert);
        }
        else if (alert.is_fatal())
        {
            pendingAlert_ = std::move(alert);
        }
        else
        {
            LOG_TRACE << "Non-fatal TLS alert received: "
                      << alert.type_string();
        }
    }

    bool dispatchPendingAlert()
    {
        if (!pendingAlert_)
            return false;

        auto guard = shared_from_this();
        auto alert = std::move(*pendingAlert_);
        pendingAlert_.reset();
        if (alert.type() == Botan::TLS::Alert::CloseNotify)
        {
            LOG_TRACE << "TLS close notify received";
            if (closeCallback_)
                closeCallback_(conn_);
            return false;
        }

        handleSSLError(sslErrorForAlert(alert.type(), tlsConnected_));
        return true;
    }

    void tls_session_activated() override
    {
        LOG_TRACE << "tls_session_activated";
        tlsConnected_ = true;
        setApplicationProtocol(channel_->application_protocol());
        if (handshakeCallback_)
            handshakeCallback_(conn_);
    }

    void tls_session_established(
        const Botan::TLS::Session_Summary &session) override
    {
        if (contextPtr_->isServer)
        {
            const auto serverName = session.server_info().hostname();
            // Resumed handshakes may not request a certificate from
            // Credentials. Select once here as well so metadata remains
            // complete and a provider can reject a resumed connection.
            if (!credsPtr_->ensure_server_credentials(serverName))
                throw Botan::TLS::TLS_Exception(
                    Botan::TLS::Alert::HandshakeFailure,
                    "No server credentials are available for this connection");
            updateServerMetadata();
        }
        else
        {
            const auto *localCertificate =
                credsPtr_->selected_client_leaf_certificate();
            if (localCertificate != nullptr)
                setLocalCertificate(
                    std::make_shared<BotanCertificate>(*localCertificate));
            else
                setLocalCertificate(nullptr);
        }
        credsPtr_->finish_handshake();
    }

    void updateServerMetadata()
    {
        setSniName(credsPtr_->selected_server_name());
        const auto *localCertificate =
            credsPtr_->selected_leaf_certificate();
        if (localCertificate != nullptr)
            setLocalCertificate(
                std::make_shared<BotanCertificate>(*localCertificate));
    }

    void tls_verify_cert_chain(
        const std::vector<Botan::X509_Certificate> &certs,
        const std::vector<std::optional<Botan::OCSP::Response>> &ocsp,
        const std::vector<Botan::Certificate_Store *> &trusted_roots,
        Botan::Usage_Type usage,
        std::string_view hostname,
        const Botan::TLS::Policy &policy) override
    {
        if (!contextPtr_->isServer)
            setSniName(std::string(hostname));
        if (contextPtr_->validate)
        {
            if (certs.empty())
                throw Botan::TLS::TLS_Exception(
                    Botan::TLS::Alert::NoCertificate,
                    "Certificate validation failed: no certificate");
            if (contextPtr_->allowBrokenChain)
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
                if (!contextPtr_->isServer && !hostname.empty() &&
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

        if (!certs.empty())
            setPeerCertificate(std::make_shared<BotanCertificate>(certs[0]));
    }

    std::shared_ptr<TrantorPolicy> validationPolicy_;
    std::shared_ptr<Credentials> credsPtr_;
    std::unique_ptr<Botan::TLS::Channel> channel_;
    bool tlsConnected_ = false;
    bool processedSslError_ = false;
    bool messageCallbackPending_ = false;
    ssize_t lastWriteSize_ = 0;
    std::optional<Botan::TLS::Alert> pendingAlert_;
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
    auto ctx = std::make_shared<SSLContext>();
    ctx->isServer = server;
    ctx->certificateProvider = policy.getServerCertificateProvider();
    ctx->alpnProtocols = policy.getAlpnProtocols();
    ctx->hostname = policy.getHostname();
    ctx->validate = policy.getValidate();
    ctx->allowBrokenChain = policy.getAllowBrokenChain();
    auto sessionRng = std::make_shared<Botan::AutoSeeded_RNG>();
    // This deliberately provides stateful TLS 1.2 resumption only. Botan's
    // in-memory manager does not issue the server tickets needed for TLS 1.3
    // resumption; changing that requires an explicit ticket-key policy.
    ctx->sessionManager =
        std::make_shared<Botan::TLS::Session_Manager_In_Memory>(sessionRng);

    if (!policy.getCertificatePem().empty())
    {
        Botan::DataSource_Memory keySource(policy.getPrivateKeyPem().empty()
                                               ? policy.getCertificatePem()
                                               : policy.getPrivateKeyPem());
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

    validateCertificateAndKey(ctx->certChain, ctx->key.get());

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
        LOG_WARN << "TLSPolicy enables old TLS, but Botan does not support "
                    "TLS/SSL below TLS 1.2. Ignoring this option.";
    if (!policy.getConfCmds().empty())
        LOG_WARN << "Botan does not support sslConfCmds; ignoring them.";
    return ctx;
}
