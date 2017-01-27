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
    TcpConnectionPtr newPtr=std::make_shared<TcpConnection>(loop_,sockfd,acceptorPtr_->addr(),peer);
    newPtr->setRecvMsgCallback([=](const TcpConnectionPtr &connectionPtr,MsgBuffer *buffer){
        //LOG_TRACE<<"recv Msg "<<buffer->readableBytes()<<" bytes";
        if(recvMessageCallback_)
            recvMessageCallback_(connectionPtr,buffer);
    });
    connSet_.insert(newPtr);
}
void TcpServer::start() {
    loop_->runInLoop([=](){
        acceptorPtr_->listen();
    });
}