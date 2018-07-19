#include <trantor/net/ssl/SSLServer.h>
using namespace trantor;
using namespace std::placeholders;
SSLServer::SSLServer(trantor::EventLoop *loop,
                     const trantor::InetAddress &address,
                     const std::string &name):
_loop(loop),
_tcpServer(std::unique_ptr<TcpServer>(new TcpServer(loop,address,name))),
_serverName(name)
{
    _tcpServer->setConnectionCallback(std::bind(&SSLServer::onTcpConnection,this,_1));
    _tcpServer->setRecvMessageCallback(std::bind(&SSLServer::onTcpRecvMessage,this,_1,_2));
    _tcpServer->setWriteCompleteCallback(std::bind(&SSLServer::onTcpWriteComplete,this,_1));
}

void SSLServer::onTcpConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        //new tcp connection
        _connMap[conn]=std::make_shared<SSLConnection>(conn);
        //init ssl
    } else
    {
        _connMap.erase(conn);
    }
}
void SSLServer::onTcpRecvMessage(const TcpConnectionPtr& conn,
                      MsgBuffer* buffer)
{

}
void SSLServer::onTcpWriteComplete(const TcpConnectionPtr& conn)
{
    assert(_connMap.find(conn)!=_connMap.end());
    _writeCompleteCallback(_connMap[conn]);
}