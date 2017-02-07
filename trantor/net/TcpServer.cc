#include <trantor/net/TcpServer.h>
#include <trantor/net/Acceptor.h>
#include <trantor/utils/Logger.h>
#include <functional>
#include <vector>
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
    //test code for blocking or nonblocking
//    std::vector<char> str(1024*1024*100);
//    for(int i=0;i<str.size();i++)
//        str[i]='A';
//    LOG_TRACE<<"vector size:"<<str.size();
//    size_t n=write(sockfd,&str[0],str.size());
//    LOG_TRACE<<"write "<<n<<" bytes";
    loop_->assertInLoopThread();
    EventLoop *ioLoop=NULL;
    if(loopPoolPtr_&&loopPoolPtr_->getLoopNum()>0)
    {
        ioLoop=loopPoolPtr_->getNextLoop();
    }
    if(ioLoop==NULL)
        ioLoop=loop_;
    TcpConnectionPtr newPtr=std::make_shared<TcpConnection>(ioLoop,sockfd,InetAddress(Socket::getLocalAddr(sockfd)),peer);
    newPtr->setRecvMsgCallback([=](const TcpConnectionPtr &connectionPtr,MsgBuffer *buffer){
        //LOG_TRACE<<"recv Msg "<<buffer->readableBytes()<<" bytes";
        if(recvMessageCallback_)
            recvMessageCallback_(connectionPtr,buffer);
    });
    newPtr->setConnectionCallback([=](const TcpConnectionPtr &connectionPtr){
        if(connectionCallback_)
            connectionCallback_(connectionPtr);
    });
    newPtr->setWriteCompleteCallback([=](const TcpConnectionPtr &connectionPtr){
        if(writeCompleteCallback_)
            writeCompleteCallback_(connectionPtr);
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
void TcpServer::setIoLoopNum(size_t num)
{
    assert(num>=0);
    loop_->runInLoop([=](){
        loopPoolPtr_=std::unique_ptr<EventLoopThreadPool>(new EventLoopThreadPool(num));
        loopPoolPtr_->start();
    });
}
void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr) {
    LOG_TRACE<<"connectionClosed";
    //loop_->assertInLoopThread();
    loop_->runInLoop([=](){
        size_t n=connSet_.erase(connectionPtr);
        assert(n==1);
    });


    connectionPtr->connectDestroyed();
}