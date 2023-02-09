#include "OpenSSLConnectionImpl.h"
#include "trantor/utils/Logger.h"
#include "trantor/utils/Utilities.h"
#include <array>

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

using namespace trantor;
using namespace std::placeholders;

namespace internal
{
#ifdef _WIN32
// Code yanked from stackoverflow
// https://stackoverflow.com/questions/9507184/can-openssl-on-windows-use-the-system-certificate-store
inline bool loadWindowsSystemCert(X509_STORE *store)
{
    auto hStore = CertOpenSystemStoreW((HCRYPTPROV_LEGACY)NULL, L"ROOT");

    if (!hStore)
    {
        return false;
    }

    PCCERT_CONTEXT pContext = NULL;
    while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) !=
           nullptr)
    {
        auto encoded_cert =
            static_cast<const unsigned char *>(pContext->pbCertEncoded);

        auto x509 = d2i_X509(NULL, &encoded_cert, pContext->cbCertEncoded);
        if (x509)
        {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
    }

    CertFreeCertificateContext(pContext);
    CertCloseStore(hStore, 0);

    return true;
}
#endif

inline bool verifyCommonName(X509 *cert, const std::string &hostname)
{
    X509_NAME *subjectName = X509_get_subject_name(cert);

    if (subjectName != nullptr)
    {
        std::array<char, BUFSIZ> name;
        auto length = X509_NAME_get_text_by_NID(subjectName,
                                                NID_commonName,
                                                name.data(),
                                                (int)name.size());
        if (length == -1)
            return false;

        return utils::verifySslName(std::string(name.begin(),
                                                name.begin() + length),
                                    hostname);
    }

    return false;
}

inline bool verifyAltName(X509 *cert, const std::string &hostname)
{
    bool good = false;
    auto altNames = static_cast<const struct stack_st_GENERAL_NAME *>(
        X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));

    if (altNames)
    {
        int numNames = sk_GENERAL_NAME_num(altNames);

        for (int i = 0; i < numNames && !good; i++)
        {
            auto val = sk_GENERAL_NAME_value(altNames, i);
            if (val->type != GEN_DNS)
            {
                LOG_WARN << "Name using IP addresses are not supported. Open "
                            "an issue if you need that feature";
                continue;
            }
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
            auto name = (const char *)ASN1_STRING_get0_data(val->d.ia5);
#else
            auto name = (const char *)ASN1_STRING_data(val->d.ia5);
#endif
            auto name_len = (size_t)ASN1_STRING_length(val->d.ia5);
            good = utils::verifySslName(std::string(name, name + name_len),
                                        hostname);
        }
    }

    GENERAL_NAMES_free((STACK_OF(GENERAL_NAME) *)altNames);
    return good;
}

static bool validatePeerCertificate(SSL *ssl,
                                    X509 *cert,
                                    const std::string &hostname,
                                    bool validateChain,
                                    bool validateDate,
                                    bool validateDomain)
{
    LOG_TRACE << "Validating peer cerificate";

    if (validateDomain)
    {
        bool domainIsValid =
            verifyCommonName(cert, hostname) || verifyAltName(cert, hostname);
        if (!domainIsValid)
            return false;
    }

    if (validateChain == false && validateDate == false)
        return true;

    auto result = SSL_get_verify_result(ssl);
    if (result == X509_V_ERR_CERT_NOT_YET_VALID ||
        result == X509_V_ERR_CERT_HAS_EXPIRED)
    {
        // What happens if cert is self-signed and expired?
        if (!validateDate)
            return true;
        LOG_TRACE << "cert error code: " << result
                  << ", date validation failed";
        return false;
    }

    if (result != X509_V_OK && validateChain)
    {
        LOG_TRACE << "cert error code: " << result;
        LOG_ERROR << "Peer certificate is not valid";
        return false;
    }

    return true;
}

static int serverSelectProtocol(SSL *ssl,
                                const unsigned char **out,
                                unsigned char *outlen,
                                const unsigned char *in,
                                unsigned int inlen,
                                void *arg)
{
    LOG_TRACE << "============================================================ "
                 "Selecting protocol";
    auto protocols = static_cast<std::vector<std::string> *>(arg);
    if (protocols->empty())
        return SSL_TLSEXT_ERR_NOACK;

    for (int i = 0; i < inlen; i++)
    {
        auto protocol = std::string((const char *)in, inlen);
        if (std::find(protocols->begin(), protocols->end(), protocol) !=
            protocols->end())
        {
            *out = (unsigned char *)in;
            *outlen = inlen;
            return SSL_TLSEXT_ERR_OK;
        }
    }
    return SSL_TLSEXT_ERR_NOACK;
}

}  // namespace internal

// Force OpenSSL to initialize before main() is called
static bool sslInitFlag = []() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
#endif
    return true;
}();

OpenSSLConnectionImpl::OpenSSLConnectionImpl(TcpConnectionPtr rawConn,
                                             SSLPolicyPtr policy,
                                             SSLContextPtr context)
    : rawConnPtr_(rawConn), policyPtr_(policy), contextPtr_(context)
{
    rawConnPtr_->setConnectionCallback(
        std::bind(&OpenSSLConnectionImpl::onConnection, this, _1));
    rawConnPtr_->setRecvMsgCallback(
        std::bind(&OpenSSLConnectionImpl::onRecvMessage, this, _1, _2));
    rawConnPtr_->setWriteCompleteCallback(
        std::bind(&OpenSSLConnectionImpl::onWriteComplete, this, _1));
    rawConnPtr_->setCloseCallback(
        std::bind(&OpenSSLConnectionImpl::onDisconnection, this, _1));

    rbio_ = BIO_new(BIO_s_mem());
    wbio_ = BIO_new(BIO_s_mem());
    ssl_ = SSL_new(contextPtr_->ctx());
    assert(ssl_);
    assert(rbio_);
    assert(wbio_);
    SSL_set_bio(ssl_, rbio_, wbio_);
    if (!policyPtr_->getHostname().empty())
        SSL_set_tlsext_host_name(ssl_, policyPtr_->getHostname().c_str());
}

OpenSSLConnectionImpl::~OpenSSLConnectionImpl()
{
    if (ssl_)
        SSL_free(ssl_);
}

void OpenSSLConnectionImpl::startClientEncryption()
{
    assert(ssl_);

    const auto &protocols = policyPtr_->getAlpnProtocols();
    if (!protocols.empty())
    {
        std::string alpnList;
        alpnList.reserve(24);  // some reasonable size
        for (const auto &proto : policyPtr_->getAlpnProtocols())
        {
            char ch = static_cast<char>(proto.size());
            alpnList.push_back(ch);
            alpnList.append(proto);
        }
        SSL_set_alpn_protos(ssl_,
                            (const unsigned char *)(alpnList.data()),
                            (unsigned int)alpnList.size());
    }
    SSL_set_connect_state(ssl_);
}

void OpenSSLConnectionImpl::startServerEncryption()
{
    assert(ssl_);

    // TODO: support ALPN
    SSL_set_accept_state(ssl_);
}

void OpenSSLConnectionImpl::onRecvMessage(const TcpConnectionPtr &conn,
                                          MsgBuffer *buffer)
{
    LOG_TRACE << "Received " << buffer->readableBytes()
              << " bytes from lower layer";
    if (buffer->readableBytes() == 0)
        return;
    while (buffer->readableBytes() > 0)
    {
        int n = BIO_write(rbio_, buffer->peek(), (int)buffer->readableBytes());
        if (n <= 0)
        {
            // TODO: make the status code more specific
            handleSSLError(SSLError::kSSLHandshakeError);
            return;
        }

        buffer->retrieve(n);
        bool need_more_data = processHandshake();
        if (need_more_data)
            break;
    }
}

void OpenSSLConnectionImpl::send(const char *msg, size_t len)
{
    int n = SSL_write(ssl_, msg, (int)len);
    if (n <= 0)
    {
        // Hope this won't happen
    }
    else
    {
        sendTLSData();
    }
}

void OpenSSLConnectionImpl::shutdown()
{
    if (SSL_is_init_finished(ssl_))
    {
        SSL_shutdown(ssl_);
        sendTLSData();
    }
    rawConnPtr_->shutdown();
}

void OpenSSLConnectionImpl::forceClose()
{
    rawConnPtr_->forceClose();
}

bool OpenSSLConnectionImpl::processHandshake()
{
    if (!SSL_is_init_finished(ssl_))
    {
        int ret = SSL_do_handshake(ssl_);
        if (ret == 1)
        {
            LOG_TRACE << "SSL handshake finished";
            if (policyPtr_->getIsServer())
            {
                const char *sniName =
                    SSL_get_servername(ssl_, TLSEXT_NAMETYPE_host_name);
                if (sniName)
                    sniName_ = sniName;
            }
            else
            {
                sniName_ = policyPtr_->getHostname();
                const unsigned char *alpn = nullptr;
                unsigned int alpnlen = 0;
                SSL_get0_alpn_selected(ssl_, &alpn, &alpnlen);
                if (alpn)
                    alpnProtocol_ = std::string((char *)alpn, alpnlen);
            }

            auto cert = SSL_get_peer_certificate(ssl_);
            if (cert)
                peerCertPtr_ = std::make_shared<OpenSSLCertificate>(cert);

            bool needCert = policyPtr_->getValidateChain() ||
                            policyPtr_->getValidateDate() ||
                            policyPtr_->getValidateDomain();
            if (needCert && !peerCertPtr_)
            {
                LOG_TRACE << "SSL handshake error: no peer certificate";
                handleSSLError(SSLError::kSSLInvalidCertificate);
                return true;
            }
            if (needCert)
            {
                bool valid = internal::validatePeerCertificate(
                    ssl_,
                    cert,
                    policyPtr_->getHostname(),
                    policyPtr_->getValidateChain(),
                    policyPtr_->getValidateDate(),
                    policyPtr_->getValidateDomain());
                if (!valid)
                {
                    LOG_TRACE
                        << "SSL handshake error: invalid peer certificate";
                    handleSSLError(SSLError::kSSLInvalidCertificate);
                    return true;
                }
            }

            connectionCallback_(shared_from_this());
            return true;
        }
        else
        {
            int err = SSL_get_error(ssl_, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            {
                LOG_TRACE << "SSL handshake wants to write";
                sendTLSData();
                return true;
            }
            else
            {
                LOG_TRACE << "SSL handshake error";
                handleSSLError(SSLError::kSSLHandshakeError);
                return true;
            }
        }
    }
    else
    {
        recvBuffer_.ensureWritableBytes(4096);
        int n = SSL_read(ssl_,
                         recvBuffer_.beginWrite(),
                         (int)recvBuffer_.writableBytes());
        if (n > 0)
        {
            recvBuffer_.hasWritten(n);
            recvMsgCallback_(shared_from_this(), &recvBuffer_);
        }
        return true;
    }
}

void OpenSSLConnectionImpl::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        LOG_TRACE << "Connection established. Start SSL handshake";

        if (policyPtr_->getIsServer())
            startServerEncryption();
        else
            startClientEncryption();

        processHandshake();
    }
}

void OpenSSLConnectionImpl::sendTLSData()
{
    void *buf = nullptr;
    int len = BIO_get_mem_data(wbio_, &buf);
    if (len > 0)
    {
        sendRawData(buf, len);
        BIO_reset(wbio_);
    }
}

void OpenSSLConnectionImpl::handleSSLError(SSLError error)
{
    rawConnPtr_->forceClose();
    sslErrorCallback_(error);
}

void OpenSSLConnectionImpl::onWriteComplete(const TcpConnectionPtr &conn)
{
    writeCompleteCallback_(shared_from_this());
}

void OpenSSLConnectionImpl::onDisconnection(const TcpConnectionPtr &conn)
{
    // ??
    closeCallback_(shared_from_this());
}

void OpenSSLConnectionImpl::onClosed(const TcpConnectionPtr &conn)
{
    closeCallback_(shared_from_this());
}

void OpenSSLConnectionImpl::onHighWaterMark(const TcpConnectionPtr &conn,
                                            size_t markLen)
{
    highWaterMarkCallback_(shared_from_this(), markLen);
}

void OpenSSLConnectionImpl::setHighWaterMarkCallback(
    const HighWaterMarkCallback &cb,
    size_t mark)
{
    highWaterMarkCallback_ = cb;
    rawConnPtr_->setHighWaterMarkCallback(
        std::bind(&OpenSSLConnectionImpl::onHighWaterMark, this, _1, _2), mark);
}

void OpenSSLConnectionImpl::sendRawData(const void *data, size_t len)
{
    LOG_TRACE << "Sending raw data: " << len << " bytes";
    rawConnPtr_->send(data, len);
}

SSLContext::SSLContext(
    bool useOldTLS,
    bool enableValidation,
    const std::vector<std::pair<std::string, std::string>> &sslConfCmds)
{
    // Ungodly amount of preprocessor macros to support older versions of
    // OpenSSL and LibreSSL
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define SSL_METHOD TLS_method
#else
#define SSL_METHOD SSLv23_method
#endif

#ifdef LIBRESSL_VERSION_NUMBER
    ctx_ = SSL_CTX_new(SSL_METHOD());
    if (ctx_ == nullptr)
        throw std::runtime_error("Failed to create SSL context");
    if (sslConfCmds.size() != 0)
        LOG_WARN << "LibreSSL does not support SSL configuration commands";

    if (!useOldTLS)
        SSL_CTX_set_min_proto_version(ctxPtr_, TLS1_2_VERSION);
#else
    ctx_ = SSL_CTX_new(SSL_METHOD());
    if (ctx_ == nullptr)
        throw std::runtime_error("Failed to create SSL context");
    SSL_CONF_CTX *cctx = SSL_CONF_CTX_new();
    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_SERVER);
    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_CLIENT);
    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_CERTIFICATE);
    SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_FILE);
    SSL_CONF_CTX_set_ssl_ctx(cctx, ctx_);
    for (const auto &cmd : sslConfCmds)
        SSL_CONF_cmd(cctx, cmd.first.data(), cmd.second.data());
    SSL_CONF_CTX_finish(cctx);
    SSL_CONF_CTX_free(cctx);
    if (useOldTLS == false)
    {
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
        SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
#else
        const auto opt = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_SSLv2 |
                         SSL_OP_NO_SSLv3;
        SSL_CTX_set_options(ctx_, opt);
#endif
    }
    else
    {
        LOG_WARN << "TLS 1.1 and below enabled. They are considered "
                    "obsolete, insecure standards and should only be "
                    "used for legacy purpose.";
    }
#endif
}

SSLContext::~SSLContext()
{
    if (ctx_)
        SSL_CTX_free(ctx_);
}

TcpConnectionPtr trantor::newTLSConnection(TcpConnectionPtr lowerConn,
                                           SSLPolicyPtr policy,
                                           SSLContextPtr ctx)
{
    return std::make_shared<OpenSSLConnectionImpl>(lowerConn, policy, ctx);
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy)
{
    auto ctx = std::make_shared<SSLContext>(policy.getUseOldTLS(),
                                            policy.getValidateChain(),
                                            policy.getConfCmds());
    if (!policy.getCertPath().empty() && !policy.getKeyPath().empty())
    {
        if (SSL_CTX_use_certificate_file(ctx->ctx(),
                                         policy.getCertPath().data(),
                                         SSL_FILETYPE_PEM) <= 0)
        {
            throw std::runtime_error("Failed to load certificate");
        }
        if (SSL_CTX_use_PrivateKey_file(ctx->ctx(),
                                        policy.getKeyPath().data(),
                                        SSL_FILETYPE_PEM) <= 0)
        {
            throw std::runtime_error("Failed to load private key");
        }
        if (SSL_CTX_check_private_key(ctx->ctx()) == 0)
        {
            throw std::runtime_error(
                "Private key does not match the "
                "certificate public key");
        }
    }
    if (policy.getValidateChain() || policy.getValidateDate() ||
        policy.getValidateDomain() && policy.getUseSystemCertStore())
    {
#ifdef _WIN32
        internal::loadWindowsSystemCert(SSL_CTX_get_cert_store(ctx->ctx()));
#else
        SSL_CTX_set_default_verify_paths(ctx->ctx());
#endif
    }

    if (!policy.getCaPath().empty())
    {
        if (SSL_CTX_load_verify_locations(ctx->ctx(),
                                          policy.getCaPath().data(),
                                          nullptr) <= 0)
        {
            throw std::runtime_error("Failed to load CA file");
        }

        STACK_OF(X509_NAME) *cert_names =
            SSL_load_client_CA_file(policy.getCaPath().data());
        if (cert_names == nullptr)
        {
            throw std::runtime_error("Failed to load CA file");
        }
        SSL_CTX_set_client_CA_list(ctx->ctx(), cert_names);
        SSL_CTX_set_verify(ctx->ctx(), SSL_VERIFY_PEER, nullptr);
    }

    if (!policy.getAlpnProtocols().empty() && policy.getIsServer())
    {
        SSL_CTX_set_alpn_select_cb(ctx->ctx(),
                                   internal::serverSelectProtocol,
                                   (void *)&policy.getAlpnProtocols());
    }
    return ctx;
}