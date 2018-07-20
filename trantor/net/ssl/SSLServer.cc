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
    //init OpenSSL
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || \
	(defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
    // Initialize OpenSSL
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif

    /* Create a new OpenSSL context */
    _sslCtxPtr=std::shared_ptr<SSL_CTX>(
            SSL_CTX_new(SSLv23_method()),
            [](SSL_CTX *ctx){
                SSL_CTX_free(ctx);
            });
    assert(_sslCtxPtr);
}

void SSLServer::onTcpConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        //new tcp connection
        _connMap[conn]=std::make_shared<SSLConnection>(conn,_sslCtxPtr);
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

void SSLServer::loadCertAndKey(const std::string &certPath, const std::string &keyPath) {

    auto r = SSL_CTX_use_certificate_file(_sslCtxPtr.get(), certPath.c_str(), SSL_FILETYPE_PEM);
    r = SSL_CTX_use_PrivateKey_file(_sslCtxPtr.get(), keyPath.c_str(), SSL_FILETYPE_PEM);
    r = SSL_CTX_check_private_key(_sslCtxPtr.get());
}