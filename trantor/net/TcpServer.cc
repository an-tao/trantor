#include <trantor/net/TcpServer.h>
#include <trantor/net/Acceptor.h>
#include <trantor/utils/Logger.h>
#include <functional>
using namespace trantor;
using namespace std::placeholders;
TcpServer::TcpServer(EventLoop *loop, const InetAddress &address, const std::string &name)
:loop_(loop),
 acceptorPtr_(new Acceptor(loop,address)),
 serverName_(name)
{
    acceptorPtr_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,_1,_2));
}
TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << serverName_ << "] destructing";

//    for (ConnectionMap::iterator it(connections_.begin());
//         it != connections_.end(); ++it)
//    {
//        TcpConnectionPtr conn = it->second;
//        it->second.reset();
//        conn->getLoop()->runInLoop(
//                boost::bind(&TcpConnection::connectDestroyed, conn));
//        conn.reset();
//    }
}
void TcpServer::newConnection(int sockfd, const InetAddress &peer) {
    LOG_TRACE<<"new connection:fd="<<sockfd<<" address="<<peer.toIpPort();
    TcpConnectionPtr newPtr=std::make_shared<TcpConnection>(loop_,sockfd,InetAddress(Socket::getLocalAddr(sockfd)),peer);
    newPtr->setRecvMsgCallback([=](const TcpConnectionPtr &connectionPtr,MsgBuffer *buffer){
        //LOG_TRACE<<"recv Msg "<<buffer->readableBytes()<<" bytes";
        if(recvMessageCallback_)
            recvMessageCallback_(connectionPtr,buffer);
    });
    newPtr->setConnectionCallback([=](const TcpConnectionPtr &connectionPtr){
        if(connectionCallback_)
            connectionCallback_(connectionPtr);
    });
    newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed,this,_1));
    connSet_.insert(newPtr);
    newPtr->connectEstablished();
}
void TcpServer::start() {
    loop_->runInLoop([=](){
        acceptorPtr_->listen();
    });
}
void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr) {
    LOG_TRACE<<"connectionClosed";
    loop_->assertInLoopThread();
    size_t n=connSet_.erase(connectionPtr);
    assert(n==1);
    connectionPtr->connectDestroyed();
}