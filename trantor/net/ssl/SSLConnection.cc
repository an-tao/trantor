#include <trantor/net/ssl/SSLConnection.h>
using namespace trantor;
SSLConnection::SSLConnection(const trantor::TcpConnectionPtr &conn,const std::shared_ptr<SSL_CTX> &ctxPtr):
_tcpConn(conn),
_sslPtr(std::shared_ptr<SSL>(SSL_new(ctxPtr.get()),[](SSL *ssl){SSL_free(ssl);}))
{

}
void SSLConnection::send(const char *msg,uint64_t len)
{

}
void SSLConnection::send(const std::string &msg)
{

}