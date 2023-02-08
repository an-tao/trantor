#pragma once

#include <trantor/net/TcpConnection.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

namespace trantor
{

struct SSLContext
{
    SSLContext(
        bool useOldTLS,
        bool enableValidation,
        const std::vector<std::pair<std::string, std::string>>& sslConfCmds);
    ~SSLContext();
    SSL_CTX* ctx_ = nullptr;

    SSL_CTX* ctx() const
    {
        return ctx_;
    }
};

}  // namespace trantor