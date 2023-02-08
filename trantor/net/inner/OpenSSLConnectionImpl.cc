#include "OpenSSLConnectionImpl.h"
#include "trantor/utils/Logger.h"

using namespace trantor;

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
    ctxPtr_ = SSL_CTX_new(SSL_METHOD());
    if (sslConfCmds.size() != 0)
        LOG_WARN << "LibreSSL does not support SSL configuration commands";

    if (!useOldTLS)
        SSL_CTX_set_min_proto_version(ctxPtr_, TLS1_2_VERSION);
#else
    ctx_ = SSL_CTX_new(SSL_METHOD());
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
    return nullptr;
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy)
{
    return std::make_shared<SSLContext>(policy.getUseOldTLS(),
                                        policy.getValidateChain(),
                                        policy.getConfCmds());
}