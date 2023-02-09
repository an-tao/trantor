#include "BotanTLSConnectionImpl.h"
#include <trantor/utils/Logger.h>
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

BotanTLSConnectionImpl::BotanTLSConnectionImpl(TcpConnectionPtr rawConn,
                                               SSLPolicyPtr policy,
                                               SSLContextPtr context)
    : rawConnPtr_(std::move(rawConn)),
      policyPtr_(std::move(policy)),
      contextPtr_(std::move(context))
{
    rawConnPtr_->setConnectionCallback(
        std::bind(&BotanTLSConnectionImpl::onConnection, this, _1));
    rawConnPtr_->setRecvMsgCallback(
        std::bind(&BotanTLSConnectionImpl::onRecvMessage, this, _1, _2));
    rawConnPtr_->setWriteCompleteCallback(
        std::bind(&BotanTLSConnectionImpl::onWriteComplete, this, _1));
    rawConnPtr_->setCloseCallback(
        std::bind(&BotanTLSConnectionImpl::onDisconnection, this, _1));
}

void BotanTLSConnectionImpl::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        LOG_TRACE
            << "Low level connection established. Starting TLS handshake.";
        if (policyPtr_->getIsServer())
            startServerEncryption();
        else
            startClientEncryption();
    }
    else
    {
        LOG_TRACE << "Low level connection closed.";
        connectionCallback_(shared_from_this());
    }
}

void BotanTLSConnectionImpl::onRecvMessage(const TcpConnectionPtr &conn,
                                           MsgBuffer *buffer)
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
        conn->shutdown();

        if (connected() == false)
            handleSSLError(SSLError::kSSLHandshakeError);
    }
    buffer->retrieveAll();
}

void BotanTLSConnectionImpl::onWriteComplete(const TcpConnectionPtr &conn)
{
    writeCompleteCallback_(shared_from_this());
}

void BotanTLSConnectionImpl::onDisconnection(const TcpConnectionPtr &conn)
{
    // ??
    closeCallback_(shared_from_this());
}

void BotanTLSConnectionImpl::onClosed(const TcpConnectionPtr &conn)
{
    closeCallback_(shared_from_this());
}

void BotanTLSConnectionImpl::onHighWaterMark(const TcpConnectionPtr &conn,
                                             size_t bytesToSent)
{
    highWaterMarkCallback_(shared_from_this(), bytesToSent);
}

void BotanTLSConnectionImpl::setHighWaterMarkCallback(
    const HighWaterMarkCallback &cb,
    size_t mark)
{
    highWaterMarkCallback_ = cb;
    rawConnPtr_->setHighWaterMarkCallback(
        std::bind(&BotanTLSConnectionImpl::onHighWaterMark, this, _1, _2),
        mark);
}

void BotanTLSConnectionImpl::send(const char *msg, size_t len)
{
    channel_->send((const uint8_t *)msg, len);
}
void BotanTLSConnectionImpl::startClientEncryption()
{
    if (policyPtr_->getConfCmds().empty() == false)
    {
        LOG_WARN << "BotanTLSConnectionImpl does not support sslConfCmds.";
    }
    credsPtr_ = std::make_unique<Credentials>(contextPtr_->key.get(),
                                              contextPtr_->cert.get(),
                                              contextPtr_->certStore.get());
    channel_ = std::make_unique<Botan::TLS::Client>(
        *this,
        sessionManager,
        *credsPtr_,
        policy_,
        rng,
        Botan::TLS::Server_Information(policyPtr_->getHostname()),
        policyPtr_->getUseOldTLS() ? Botan::TLS::Protocol_Version::TLS_V10
                                   : Botan::TLS::Protocol_Version::TLS_V12,
        policyPtr_->getAlpnProtocols());
}

void BotanTLSConnectionImpl::startServerEncryption()
{
    if (policyPtr_->getConfCmds().empty() == false)
    {
        LOG_WARN << "BotanTLSConnectionImpl does not support sslConfCmds.";
    }

    credsPtr_ = std::make_unique<Credentials>(contextPtr_->key.get(),
                                              contextPtr_->cert.get(),
                                              contextPtr_->certStore.get());
    channel_ = std::make_unique<Botan::TLS::Server>(
        *this, sessionManager, *credsPtr_, policy_, rng);
}

void BotanTLSConnectionImpl::tls_emit_data(const uint8_t data[], size_t size)
{
    LOG_TRACE << "tls_emit_data: sending " << size << " bytes";
    rawConnPtr_->send(data, size);
}

void BotanTLSConnectionImpl::tls_record_received(uint64_t /*seq*/,
                                                 const uint8_t data[],
                                                 size_t size)
{
    LOG_TRACE << "tls_record_received: received " << size << " bytes";
    recvBuffer_.append((const char *)data, size);
    recvMsgCallback_(shared_from_this(), &recvBuffer_);
}

std::string BotanTLSConnectionImpl::tls_server_choose_app_protocol(
    const std::vector<std::string> &client_protos)
{
    assert(policyPtr_->getIsServer());
    const auto &serverProtos = policyPtr_->getAlpnProtocols();
    if (serverProtos.empty() || client_protos.empty())
        return "";
    // usually protocols have only a few elements, so N^2 is fine
    // (and actually faster than using a set)
    for (const auto &clientProto : client_protos)
    {
        for (const auto &serverProto : serverProtos)
        {
            if (clientProto == serverProto)
                return clientProto;
        }
    }
    throw Botan::TLS::Alert(Botan::TLS::Alert::NO_APPLICATION_PROTOCOL);
}

void BotanTLSConnectionImpl::tls_alert(Botan::TLS::Alert alert)
{
    LOG_TRACE << "tls_alert: " << alert.type_string();
    if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        closingTLS_ = true;
        rawConnPtr_->shutdown();
    }
    // TODO: handle other alerts
}

void BotanTLSConnectionImpl::handleSSLError(SSLError err)
{
    if (!sslErrorCallback_)
        return;
    getLoop()->queueInLoop([this]() {
        sslErrorCallback_(SSLError::kSSLInvalidCertificate);
        channel_->close();
    });
}

bool BotanTLSConnectionImpl::tls_session_established(
    const Botan::TLS::Session &session)
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

    rawConnPtr_->getLoop()->queueInLoop(
        [this]() { connectionCallback_(shared_from_this()); });
    // Do we want to cache all sessions?
    return true;
}

void BotanTLSConnectionImpl::tls_verify_cert_chain(
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

void BotanTLSConnectionImpl::shutdown()
{
    channel_->close();
}

void BotanTLSConnectionImpl::forceClose()
{
    channel_->close();
    rawConnPtr_->forceClose();
}

std::string BotanTLSConnectionImpl::applicationProtocol() const
{
    return channel_->application_protocol();
}

TcpConnectionPtr trantor::newTLSConnection(TcpConnectionPtr lowerConn,
                                           SSLPolicyPtr policy,
                                           SSLContextPtr ctx)
{
    return std::make_shared<BotanTLSConnectionImpl>(std::move(lowerConn),
                                                    std::move(policy),
                                                    std::move(ctx));
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy)
{
    auto ctx = std::make_shared<SSLContext>();
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

    if (!policy.getCaPath().empty())
    {
        ctx->certStore = std::make_unique<Botan::Flatfile_Certificate_Store>(
            policy.getCaPath());
    }
    else if (policy.getUseSystemCertStore())
    {
        ctx->certStore = std::make_unique<Botan::System_Certificate_Store>();
    }

    if (policy.getUseOldTLS())
        LOG_WARN << "SSLPloicy have set useOldTLS to true. BUt Botan does not "
                    "support TLS/SSL below TLS 1.2. Ignoreing this option.";
    return ctx;
}
