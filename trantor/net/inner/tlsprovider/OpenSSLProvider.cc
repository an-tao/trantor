#include <trantor/utils/Logger.h>
#include <trantor/utils/Utilities.h>
#include <trantor/net/TcpConnection.h>
#include <trantor/net/inner/TLSProvider.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>

#include <fstream>
#include <mutex>
#include <list>
#include <unordered_map>
#include <array>

using namespace trantor;

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
    auto protocols = static_cast<std::vector<std::string> *>(arg);
    if (protocols->empty())
        return SSL_TLSEXT_ERR_NOACK;

    for (auto &protocol : *protocols)
    {
        const unsigned char *cur = in;
        const unsigned char *end = in + inlen;
        while (cur < end)
        {
            unsigned int len = *cur++;
            if (cur + len > end)
            {
                LOG_ERROR << "Client provided invalid protocol list in APLN";
                return SSL_TLSEXT_ERR_NOACK;
            }
            if (protocol.size() == len &&
                memcmp(cur, protocol.data(), len) == 0)
            {
                *out = cur;
                *outlen = len;
                LOG_TRACE << "Selected protocol: " << protocol;
                return SSL_TLSEXT_ERR_OK;
            }
        }
    }

    return SSL_TLSEXT_ERR_NOACK;
}

}  // namespace internal

namespace trantor
{
struct SSLContext
{
    SSLContext(
        bool useOldTLS,
        bool enableValidation,
        const std::vector<std::pair<std::string, std::string>> &sslConfCmds,
        bool server);
    ~SSLContext();
    SSL_CTX *ctx_ = nullptr;

    SSL_CTX *ctx() const
    {
        return ctx_;
    }

    bool isServer = false;
};

struct OpenSSLCertificate : public Certificate
{
    OpenSSLCertificate(X509 *cert) : cert_(cert)
    {
        assert(cert_);
    }
    ~OpenSSLCertificate()
    {
        X509_free(cert_);
    }
    virtual std::string sha1Fingerprint() const override
    {
        std::string sha1;
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int n = 0;
        if (X509_digest(cert_, EVP_sha1(), md, &n))
        {
            sha1.resize(n * 3);
            for (unsigned int i = 0; i < n; i++)
            {
                snprintf(&sha1[i * 3], 4, "%02X:", md[i]);
            }
            sha1.resize(sha1.size() - 1);
        }
        else
        {
            // handle error
            // LOG_ERROR << "X509_digest failed";
        }
        return sha1;
    }

    virtual std::string sha256Fingerprint() const override
    {
        std::string sha256;
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int n = 0;
        if (X509_digest(cert_, EVP_sha256(), md, &n))
        {
            sha256.resize(n * 3);
            for (unsigned int i = 0; i < n; i++)
            {
                snprintf(&sha256[i * 3], 4, "%02X:", md[i]);
            }
            sha256.resize(sha256.size() - 1);
        }
        else
        {
            // handle error
            // LOG_ERROR << "X509_digest failed";
        }
        return sha256;
    }

    virtual std::string pem() const override
    {
        std::string pem;
        BIO *bio = BIO_new(BIO_s_mem());
        if (bio)
        {
            PEM_write_bio_X509(bio, cert_);
            char *data = nullptr;
            long len = BIO_get_mem_data(bio, &data);
            if (len > 0)
            {
                pem.assign(data, len);
            }
            else
            {
                // handle error
                // LOG_ERROR << "BIO_get_mem_data failed";
            }
            BIO_free(bio);
        }
        else
        {
            // handle error
            // LOG_ERROR << "BIO_new failed";
        }
        return pem;
    }
    X509 *cert_ = nullptr;
};

class SessionManager
{
    struct SessionData
    {
        SSL_SESSION *session = nullptr;
        std::string key;
        TimerId timerId = 0;
        EventLoop *loop = nullptr;
    };

  public:
    ~SessionManager()
    {
        for (auto &session : sessions_)
        {
            SSL_SESSION_free(session.session);
        }
    }

    void store(const std::string &hostname,
               InetAddress peerAddr,
               SSL_SESSION *session,
               EventLoop *loop)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto key = toKey(hostname, peerAddr);
            auto it = sessionMap_.find(key);
            if (it != sessionMap_.end())
            {
                SSL_SESSION_free(it->second->session);
                it->second->loop->invalidateTimer(it->second->timerId);
                sessions_.erase(it->second);
                sessionMap_.erase(it);
            }

            SSL_SESSION_up_ref(session);
            TimerId tid = loop->runAfter(sessionTimeout_, [this, key]() {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = sessionMap_.find(key);
                if (it != sessionMap_.end())
                {
                    SSL_SESSION_free(it->second->session);
                    sessions_.erase(it->second);
                    sessionMap_.erase(it);
                }
            });
            sessions_.push_front(SessionData{session, key, tid, loop});
            sessionMap_[key] = sessions_.begin();
        }
        removeExcessSession();
    }

    SSL_SESSION *get(const std::string &hostname, InetAddress peerAddr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = toKey(hostname, peerAddr);
        auto it = sessionMap_.find(key);
        if (it != sessionMap_.end())
        {
            return it->second->session;
        }
        return nullptr;
    }

    void removeExcessSession()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sessions_.size() < maxSessions_ + mexExtendSize_)
            return;
        if (sessions_.size() > maxSessions_)
        {
            auto it = sessions_.end();
            it--;
            SSL_SESSION_free(it->session);
            it->loop->invalidateTimer(it->timerId);
            sessionMap_.erase(it->key);
            sessions_.erase(it);
        }
    }

    std::string toKey(const std::string &hostname, InetAddress peerAddr)
    {
        return hostname + peerAddr.toIpPort();
    }

    std::mutex mutex_;
    int maxSessions_ = 150;
    int mexExtendSize_ = 20;
    int sessionTimeout_ = 3600;
    std::list<SessionData> sessions_;
    std::unordered_map<std::string, std::list<SessionData>::iterator>
        sessionMap_;
};

}  // namespace trantor

static SessionManager sessionManager;

struct OpenSSLProvider : public TLSProvider, public NonCopyable
{
    OpenSSLProvider(EventLoop *loop,
                    TcpConnection *conn,
                    SSLPolicyPtr policy,
                    SSLContextPtr ctx)
        : TLSProvider(loop, conn, policy), policyPtr_(policy), contextPtr_(ctx)
    {
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

    virtual ~OpenSSLProvider()
    {
        SSL_free(ssl_);
    }

    virtual void startEncryption() override
    {
        if (contextPtr_->isServer)
        {
            assert(ssl_);
            SSL_set_accept_state(ssl_);
        }
        else
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

            SSL_SESSION *cachedSession = NULL;
            // sessionManager.get(policyPtr_->getHostname(),
            // rawConnPtr_->peerAddr());
            if (cachedSession)
            {
                SSL_set_session(ssl_, cachedSession);
            }
            SSL_set_connect_state(ssl_);
        }

        processHandshake();
        // if (buf.readableBytes() > 0)
        //     onRecvMessage(rawConnPtr_, &buf);
    }

    virtual void recvData(MsgBuffer *buffer)
    {
        LOG_TRACE << "Received " << buffer->readableBytes()
                  << " bytes from lower layer";
        if (buffer->readableBytes() == 0)
            return;
        while (buffer->readableBytes() > 0)
        {
            int n =
                BIO_write(rbio_, buffer->peek(), (int)buffer->readableBytes());
            if (n <= 0)
            {
                // TODO: make the status code more specific
                handleSSLError(SSLError::kSSLHandshakeError);
                return;
            }

            buffer->retrieve(n);

            if (!SSL_is_init_finished(ssl_))
            {
                bool handshakeDone = processHandshake();
                if (handshakeDone)
                    processApplicationData();
            }
            else
            {
                processApplicationData();
            }
        }
    }

    virtual void sendData(const char *data, size_t len)
    {
        int n = SSL_write(ssl_, data, (int)len);
        if (n <= 0)
        {
            handleSSLError(SSLError::kSSLProtocolError);
            return;
        }
        sendTLSData();
    }

    bool processHandshake()
    {
        int ret = SSL_do_handshake(ssl_);
        if (ret == 1)
        {
            LOG_TRACE << "SSL handshake finished";
            if (contextPtr_->isServer)
            {
                const char *sniName =
                    SSL_get_servername(ssl_, TLSEXT_NAMETYPE_host_name);
                if (sniName)
                    sniName_ = sniName;

                const unsigned char *alpn = nullptr;
                unsigned int alpnlen = 0;
                SSL_get0_alpn_selected(ssl_, &alpn, &alpnlen);
                if (alpn)
                    alpnProtocol_ = std::string((char *)alpn, alpnlen);
            }
            else
            {
                sniName_ = policyPtr_->getHostname();
                const unsigned char *alpn = nullptr;
                unsigned int alpnlen = 0;
                SSL_get0_alpn_selected(ssl_, &alpn, &alpnlen);
                if (alpn)
                    alpnProtocol_ = std::string((char *)alpn, alpnlen);

                SSL_SESSION *session = SSL_get0_session(ssl_);
                assert(session);
                if (SSL_SESSION_is_resumable(session))
                {
                    // TODO: store session
                    // bool reused = SSL_session_reused(ssl_) == 1;
                    // if (reused == 0)
                    //     sessionManager.store(sniName_,
                    //                         rawConnPtr_->peerAddr(),
                    //                         session,
                    //                         loop_);
                }
            }

            auto cert = SSL_get_peer_certificate(ssl_);
            if (cert)
                peerCertPtr_ = std::make_shared<OpenSSLCertificate>(cert);

            bool needCert = policyPtr_->getValidateChain() ||
                            policyPtr_->getValidateDate() ||
                            policyPtr_->getValidateDomain();
            if (needCert && !peerCertPtr_)
            {
                LOG_TRACE << "SSL handshake error: no peer certificate. Cannot "
                             "perform validation";
                handleSSLError(SSLError::kSSLInvalidCertificate);
                return false;
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
                    return false;
                }
            }

            // if (policyPtr_->getOneShotConnctionCallback() && !oneshotCalled_)
            // {
            //     oneshotCalled_ = true;
            //     policyPtr_->getOneShotConnctionCallback()(shared_from_this());
            // }
            /*else */ if (handshakeCallback_)
                handshakeCallback_(conn_);
            sendTLSData();  // Needed to send ChangeCipherSpec
            return true;
        }
        else
        {
            int err = SSL_get_error(ssl_, ret);
            if (err == SSL_ERROR_WANT_READ)
            {
                LOG_TRACE << "SSL handshake wants to read";
                sendTLSData();
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                LOG_TRACE << "SSL handshake wants to write";
                sendTLSData();
            }
            else
            {
                LOG_TRACE << "SSL handshake error";
                handleSSLError(SSLError::kSSLHandshakeError);
            }
        }
        return false;
    }

    void processApplicationData()
    {
        int n;
        int pending = SSL_pending(ssl_);
        if (pending > 0)
            recvBuffer_.ensureWritableBytes(pending);

        do
        {
            n = SSL_read(ssl_,
                         recvBuffer_.beginWrite(),
                         (int)recvBuffer_.writableBytes());
            if (n > 0)
            {
                recvBuffer_.hasWritten(n);
                LOG_TRACE << "Received " << n << " bytes from SSL";
                if (messageCallback_)
                    messageCallback_(conn_, &recvBuffer_);
            }
            else if (n <= 0)
            {
                int err = SSL_get_error(ssl_, n);
                if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL)
                {
                    LOG_TRACE << "Fatal SSL error. Close connection.";
                    handleSSLError(SSLError::kSSLProtocolError);
                }
            }
        } while (n > 0);
    }

    void sendTLSData()
    {
        MsgBuffer buf;
        int n = BIO_pending(wbio_);
        if (n > 0)
        {
            buf.ensureWritableBytes(n);
            n = BIO_read(wbio_, buf.beginWrite(), n);
            if (n > 0)
            {
                buf.hasWritten(n);
                writeCallback_(conn_, buf);
            }
        }
    }

    void handleSSLError(SSLError error)
    {
        sendTLSData();
        if (errorCallback_)
            errorCallback_(conn_, error);
    }

    SSL *ssl_;
    BIO *rbio_;
    BIO *wbio_;
    const SSLPolicyPtr policyPtr_;
    const SSLContextPtr contextPtr_;
    MsgBuffer recvBuffer_;
    std::string sniName_;
    std::string alpnProtocol_;
    CertificatePtr peerCertPtr_;
};

std::unique_ptr<TLSProvider> trantor::newTLSProvider(EventLoop *loop,
                                                     TcpConnection *conn,
                                                     SSLPolicyPtr policy,
                                                     SSLContextPtr ctx)
{
    return std::make_unique<OpenSSLProvider>(loop,
                                             conn,
                                             std::move(policy),
                                             std::move(ctx));
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy, bool isServer)
{
    auto ctx = std::make_shared<SSLContext>(policy.getUseOldTLS(),
                                            policy.getValidateChain(),
                                            policy.getConfCmds(),
                                            isServer);
    if (!policy.getCertPath().empty() && !policy.getKeyPath().empty())
    {
        if (SSL_CTX_use_certificate_file(ctx->ctx(),
                                         policy.getCertPath().data(),
                                         SSL_FILETYPE_PEM) <= 0)
        {
            throw std::runtime_error("Failed to load certificate " +
                                     policy.getCertPath());
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
    if ((policy.getValidateChain() || policy.getValidateDate() ||
         policy.getValidateDomain()) &&
        policy.getUseSystemCertStore())
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
            throw std::runtime_error("Not CA names found in file");
        }
        SSL_CTX_set_client_CA_list(ctx->ctx(), cert_names);
        SSL_CTX_set_verify(ctx->ctx(), SSL_VERIFY_PEER, nullptr);
    }

    if (!policy.getAlpnProtocols().empty() && isServer)
    {
        SSL_CTX_set_alpn_select_cb(ctx->ctx(),
                                   internal::serverSelectProtocol,
                                   (void *)&policy.getAlpnProtocols());
    }

    if (!isServer)
    {
        // We have our own session cache, so disable OpenSSL's
        SSL_CTX_set_session_cache_mode(ctx->ctx(), SSL_SESS_CACHE_OFF);
    }
    return ctx;
}