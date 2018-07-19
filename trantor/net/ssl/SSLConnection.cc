#include <trantor/net/ssl/SSLConnection.h>
using namespace trantor;
SSLConnection::SSLConnection(const trantor::TcpConnectionPtr &conn):
_tcpConn(conn)
{

}
void SSLConnection::send(const char *msg,uint64_t len)
{

}
void SSLConnection::send(const std::string &msg)
{

}