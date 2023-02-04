#include "BotanTLSConnectionImpl.h"
#include <trantor/utils/Logger.h>
#include "TcpConnectionImpl.h"

#include <sstream>
#include <iomanip>

using namespace trantor;
using namespace std::placeholders;

BotanTLSConnectionImpl::BotanTLSConnectionImpl(TcpConnectionPtr rawConn)
    : rawConnPtr_(std::move(rawConn)), sessionManager_(rng_)
{
    rawConnPtr_->setConnectionCallback(std::bind(&BotanTLSConnectionImpl::onConnection, this, _1));
    rawConnPtr_->setRecvMsgCallback(std::bind(&BotanTLSConnectionImpl::onRecvMessage, this, _1, _2));
    rawConnPtr_->setWriteCompleteCallback(std::bind(&BotanTLSConnectionImpl::onWriteComplete, this, _1));
    rawConnPtr_->setCloseCallback(std::bind(&BotanTLSConnectionImpl::onDisconnection, this, _1));
}

void BotanTLSConnectionImpl::onConnection(const TcpConnectionPtr &conn)
{
    if(conn->connected())
    {
        LOG_TRACE << "Low level connection established. Starting TLS handshake.";
        rawConnPtr_->setContext(shared_from_this());
        startClientEncryption([]{}, false, true);
    }
    else
    {
        LOG_TRACE << "Low level connection closed.";
        rawConnPtr_->getLoop()->queueInLoop([this]() {
            rawConnPtr_->setContext(nullptr);
        });
    }
}

void BotanTLSConnectionImpl::onRecvMessage(const TcpConnectionPtr &conn, MsgBuffer *buffer)
{
    LOG_TRACE << "Low level connection received " << buffer->readableBytes() << " bytes.";
    client_->received_data((const uint8_t*)buffer->peek(), buffer->readableBytes());
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
    client_->send((const uint8_t*)msg, len);
}
void BotanTLSConnectionImpl::startClientEncryption(
        std::function<void()> callback,
        bool useOldTLS,
        bool validateCert,
        std::string hostname,
        const std::vector<std::pair<std::string, std::string>>&)
{
    client_ = std::make_unique<Botan::TLS::Client>(*this, sessionManager_, creds_, policy_, rng_,
                              Botan::TLS::Server_Information(hostname), useOldTLS ? Botan::TLS::Protocol_Version::TLS_V10 : Botan::TLS::Protocol_Version::latest_tls_version());
}

void BotanTLSConnectionImpl::tls_emit_data(const uint8_t data[], size_t size)
{
    LOG_TRACE << "tls_emit_data: sending " << size << " bytes";
    rawConnPtr_->send(data, size);
}

void BotanTLSConnectionImpl::tls_record_received(uint64_t, const uint8_t data[], size_t size)
{
    LOG_TRACE << "tls_record_received: received " << size << " bytes";
    recvBuffer_.append((const char*)data, size);
    recvMsgCallback_(shared_from_this(), &recvBuffer_);
}

void BotanTLSConnectionImpl::tls_alert(Botan::TLS::Alert alert)
{
    LOG_TRACE << "tls_alert: " << alert.type_string();
    if(alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        closingTLS_ = true;
        rawConnPtr_->shutdown();
    }
}

bool BotanTLSConnectionImpl::tls_session_established(const Botan::TLS::Session& session)
{
    LOG_TRACE << "tls_session_established.";
    rawConnPtr_->getLoop()->queueInLoop([this]() {
        connectionCallback_(shared_from_this());
    });
    return true;
}

void BotanTLSConnectionImpl::shutdown()
{
    // client_->close();
    rawConnPtr_->shutdown();
}