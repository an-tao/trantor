#include <trantor/net/TcpConnection.h>
#include <trantor/net/Socket.h>
#include <trantor/net/Channel.h>
#define FETCH_SIZE 2048;
using namespace trantor;
TcpConnection::TcpConnection(EventLoop *loop, int socketfd,const InetAddress& localAddr,
                             const InetAddress& peerAddr):
        loop_(loop),
        ioChennelPtr_(new Channel(loop,socketfd)),
        sockfd_(socketfd),
        localAddr_(localAddr),
        peerAddr_(peerAddr)
{
    ioChennelPtr_->setReadCallback(std::bind(&TcpConnection::readCallback,this));
    ioChennelPtr_->enableReading();
}
void TcpConnection::readCallback() {
    //LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    int ret=0;

    size_t n=buffer_.readFd(sockfd_,&ret);
    //LOG_TRACE<<"read "<<n<<" bytes from socket";
    if(n==0)
    {
        //socket closed by peer
        //fixed me
    }
    else if(n<0)
    {
        LOG_SYSERR<<"read socket error";
    }

    if(n>0&&recvMsgCallback_)
    {
        recvMsgCallback_(shared_from_this(),&buffer_);
    }
}