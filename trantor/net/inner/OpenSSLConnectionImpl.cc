#include "OpenSSLConnectionImpl.h"

using namespace trantor;

TcpConnectionPtr trantor::newTLSConnection(TcpConnectionPtr lowerConn,
                                           SSLPolicyPtr policy,
                                           SSLContextPtr ctx)
{
    return nullptr;
}

SSLContextPtr trantor::newSSLContext(const SSLPolicy &policy)
{
    return nullptr;
}