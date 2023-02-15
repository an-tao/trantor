#pragma once

#include "TcpConnection.h"

#include <memory>

namespace trantor
{

struct TlsInitiator
{
    virtual ~TlsInitiator() = default;
    virtual void startEncryption(const TcpConnectionPtr& conn,
                                 SSLPolicyPtr policy) = 0;
};
}  // namespace trantor