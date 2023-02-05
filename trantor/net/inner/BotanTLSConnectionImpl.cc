#include "BotanTLSConnectionImpl.h"
#include <trantor/utils/Logger.h>

using namespace trantor;
using namespace std::placeholders;

BotanTLSConnectionImpl::BotanTLSConnectionImpl(TcpConnectionPtr rawConn, std::shared_ptr<SSLPolicy> policy)
    : rawConnPtr_(std::move(rawConn)), sessionManager_(rng_), policyPtr_(std::move(policy))
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
    channel_->received_data((const uint8_t *)buffer->peek(),
                           buffer->readableBytes());
    buffer->retrieveAll();
}

void BotanTLSConnectionImpl::onWriteComplete(const TcpConnectionPtr &conn)
{
    writeCompleteCallback_(shared_from_this());
}

void BotanTLSConnectionImpl::onDisconnection(const TcpConnectionPtr &conn)
{
    closeCallback_(shared_from_this());
}

void BotanTLSConnectionImpl::onClosed(const TcpConnectionPtr &conn)
{
    closeCallback_(shared_from_this());
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

    channel_ = std::make_unique<Botan::TLS::Client>(
        *this,
        sessionManager_,
        creds_,
        policy_,
        rng_,
        Botan::TLS::Server_Information(policyPtr_->getHostname()),
        policyPtr_->getUseOldTLS() ? Botan::TLS::Protocol_Version::TLS_V10
                  : Botan::TLS::Protocol_Version::TLS_V12);
}

// void BotanTLSConnectionImpl::startServerEncryption(const std::shared_ptr<SSLContext> &ctx,
//                             std::function<void()> callback)
// {
//     throw std::runtime_error("BotanTLSConnectionImpl does not support server mode. yet.");
// }

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

void BotanTLSConnectionImpl::handleCertValidationFail(SSLError err)
{
    getLoop()->queueInLoop([this]() {
        sslErrorCallback_(SSLError::kSSLInvalidCertificate);
        channel_->close();
    });
}

bool BotanTLSConnectionImpl::tls_session_established(
    const Botan::TLS::Session &session)
{
    LOG_TRACE << "tls_session_established. Starting certificate validation.";
    bool needCerts = policyPtr_->getValidateDate() || policyPtr_->getValidateDomain();
    const auto& certs = session.peer_certs();
    if(needCerts && session.peer_certs().empty()) {
        handleCertValidationFail(SSLError::kSSLInvalidCertificate);
        return false;
    }
    auto& cert = certs[0];
    if(policyPtr_->getValidateDomain() && cert.matches_dns_name(policyPtr_->getHostname()) == false) {
        handleCertValidationFail(SSLError::kSSLInvalidCertificate);
        return false;
    }
    if(policyPtr_->getValidateDate()) {
        auto notBefore = cert.not_before();
        auto notAfter = cert.not_after();
        auto now = Botan::ASN1_Time(std::chrono::system_clock::now());
        if(now < notBefore || now > notAfter) {
            handleCertValidationFail(SSLError::kSSLInvalidCertificate);
            return false;
        }
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
    if(policyPtr_->getValidateChain())
        Botan::TLS::Callbacks::tls_verify_cert_chain(certs, ocsp, trusted_roots, usage, hostname, policy);
}

void BotanTLSConnectionImpl::shutdown()
{
    channel_->close();
}

void BotanTLSConnectionImpl::forceClose()
{
    channel_->close();
    getLoop()->queueInLoop([this]() { rawConnPtr_->forceClose(); });
}