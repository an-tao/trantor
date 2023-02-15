#pragma once

#include <trantor/net/TcpConnection.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include <fstream>
#include <mutex>
#include <list>
#include <unordered_map>

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

class OpenSSLConnectionImpl
    : public TcpConnection,
      public std::enable_shared_from_this<OpenSSLConnectionImpl>
{
  public:
    OpenSSLConnectionImpl(TcpConnectionPtr rawConn,
                          SSLPolicyPtr policy,
                          SSLContextPtr context);
    virtual ~OpenSSLConnectionImpl();
    void sendRawData(const void *msg, size_t len);
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
        // barebone implementation
        std::ifstream ifs(fileName, std::ios::binary);
        if (!ifs.is_open())
        {
            throw std::runtime_error("Cannot open file");
        }
        ifs.seekg(offset);
        if (length == 0)
        {
            ifs.seekg(0, std::ios::end);
            length = ifs.tellg();
        }
        ifs.seekg(offset);
        std::vector<char> buffer(length);
        ifs.read(buffer.data(), length);
        send(buffer.data(), length);
    }
    virtual void sendFile(const wchar_t *fileName,
                          size_t offset = 0,
                          size_t length = 0) override
    {
        throw std::runtime_error("Not implemented sendFile(wstring)");
    }
    virtual void sendStream(
        std::function<std::size_t(char *, std::size_t)> callback) override
    {
        throw std::runtime_error("Not implemented sendStream()");
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

    virtual std::string applicationProtocol() const override
    {
        return alpnProtocol_;
    }

    virtual void startEncryption(SSLPolicyPtr) override
    {
        throw std::runtime_error("Cannot encrypt an encrypted connection");
    }

    virtual void startHandshake(MsgBuffer &existingData) override;

  protected:
    void startClientSideEncryption();
    void startServerSideEncryption();

    bool processHandshake();
    void processApplicationData();
    void sendTLSData();

    TcpConnectionPtr rawConnPtr_;

    void onRecvMessage(const TcpConnectionPtr &conn, MsgBuffer *buffer);
    void onConnection(const TcpConnectionPtr &conn);
    void onWriteComplete(const TcpConnectionPtr &conn);
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

    CertificatePtr peerCertPtr_;
    std::string sniName_;
    std::string alpnProtocol_;
    MsgBuffer recvBuffer_;
    SSL *ssl_ = nullptr;
    BIO *rbio_ = nullptr;
    BIO *wbio_ = nullptr;
    // TODO: Rename this to avoid confusion
    const SSLPolicyPtr policyPtr_;
    const SSLContextPtr contextPtr_;

    bool closingTLS_ = false;
};

}  // namespace trantor