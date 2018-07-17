#include <trantor/net/TcpConnection.h>
#include "Socket.h"
#include "Channel.h"
#define FETCH_SIZE 2048;
#define SEND_ORDER 1
using namespace trantor;

void trantor::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->lobalAddr().toIpPort() << " -> "
              << conn->peerAddr().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void trantor::defaultMessageCallback(const TcpConnectionPtr&,
                                        MsgBuffer* buf)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, int socketfd,const InetAddress& localAddr,
                             const InetAddress& peerAddr):
        loop_(loop),
        ioChennelPtr_(new Channel(loop,socketfd)),
        socketPtr_(new Socket(socketfd)),
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        state_(Connecting)
{
    LOG_TRACE<<"new connection:"<<peerAddr.toIpPort()<<"->"<<localAddr.toIpPort();
    ioChennelPtr_->setReadCallback(std::bind(&TcpConnection::readCallback,this));
    ioChennelPtr_->setWriteCallback(std::bind(&TcpConnection::writeCallback,this));
    ioChennelPtr_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    ioChennelPtr_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
    socketPtr_->setKeepAlive(true);
    name_=localAddr.toIpPort()+"--"+peerAddr.toIpPort();
}
TcpConnection::~TcpConnection() {

}
void TcpConnection::readCallback() {
    LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    int ret=0;

    ssize_t n=readBuffer_.readFd(socketPtr_->fd(),&ret);
    //LOG_TRACE<<"read "<<n<<" bytes from socket";
    if(n==0)
    {
        //socket closed by peer
        handleClose();
    }
    else if(n<0)
    {
        LOG_SYSERR<<"read socket error";
    }

    if(n>0&&recvMsgCallback_)
    {
        recvMsgCallback_(shared_from_this(),&readBuffer_);
    }
}
void TcpConnection::writeCallback() {
    LOG_TRACE<<"write Callback";
    loop_->assertInLoopThread();
    if(ioChennelPtr_->isWriting())
    {
        if(writeBuffer_.readableBytes()<=0)
        {
            ioChennelPtr_->disableWriting();
            //
            if(writeCompleteCallback_)
                writeCompleteCallback_(shared_from_this());
            if(state_==Disconnecting)
            {
                socketPtr_->closeWrite();
            }
        }
        else
        {
            size_t n=write(socketPtr_->fd(),writeBuffer_.peek(),writeBuffer_.readableBytes());
            writeBuffer_.retrieve(n);
//            if(writeBuffer_.readableBytes()==0)
//                ioChennelPtr_->disableWriting();
        }
    } else
    {
        LOG_SYSERR<<"no writing but call write callback";
    }
}
void TcpConnection::connectEstablished() {
    //loop_->assertInLoopThread();
    loop_->runInLoop([=](){
        LOG_TRACE<<"connectEstablished";
        assert(state_==Connecting);
        ioChennelPtr_->tie(shared_from_this());
        ioChennelPtr_->enableReading();
        state_=Connected;
        if(connectionCallback_)
            connectionCallback_(shared_from_this());
    });

}
void TcpConnection::handleClose() {
    LOG_TRACE<<"connection closed";
    loop_->assertInLoopThread();
    state_=Disconnected;
    ioChennelPtr_->disableAll();
    auto guardThis=shared_from_this();
    if(connectionCallback_)
        connectionCallback_(guardThis);
    if(closeCallback_)
    {
        LOG_TRACE<<"to call close callback";
        closeCallback_(guardThis);
    }

}
void TcpConnection::handleError() {
    int err = socketPtr_->getSocketError();
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
void TcpConnection::setTcpNoDelay(bool on)
{
    socketPtr_->setTcpNoDelay(on);
}
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == Connected)
    {
        state_=Disconnected;
        ioChennelPtr_->disableAll();

        connectionCallback_(shared_from_this());
    }
    ioChennelPtr_->remove();
}
void TcpConnection::shutdown() {
    loop_->runInLoop([=](){
        if(state_==Connected)
        {
            state_=Disconnecting;
            if(!ioChennelPtr_->isWriting())
            {
                socketPtr_->closeWrite();
            }
        }
    });
}

void TcpConnection::forceClose()
{
    loop_->runInLoop([=](){
        if (state_ == Connected || state_ == Disconnecting)
        {
            state_=Disconnecting;
            handleClose();
        }
    });
}


void TcpConnection::sendInLoop(const std::string &msg)
{
    LOG_TRACE<<"send in loop";
    loop_->assertInLoopThread();
    if(state_!=Connected)
    {
        LOG_WARN<<"Connection is not connected,give up sending";
        return;
    }
    size_t remainLen=msg.length();
    ssize_t sendLen=0;
    if(!ioChennelPtr_->isWriting()&&writeBuffer_.readableBytes()==0)
    {
        //send directly
        sendLen=write(socketPtr_->fd(),msg.c_str(),msg.length());
        if(sendLen<0)
        {
            //error
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    return;
                }
                LOG_SYSERR<< "Unexpected error("<<errno<<")";
                return;
            }
            sendLen = 0;
        }
        remainLen-=sendLen;
    }
    if(remainLen>0)
    {
        writeBuffer_.append(msg.c_str()+sendLen,remainLen);
        if(!ioChennelPtr_->isWriting())
            ioChennelPtr_->enableWriting();
        if(highWaterMarkCallback_&&writeBuffer_.readableBytes()>highWaterMarkLen_)
        {
            highWaterMarkCallback_(shared_from_this(),writeBuffer_.readableBytes());
        }
    }

}
void TcpConnection::send(const char *msg,uint64_t len){
    //fix me!
    //Need to be more efficient
    send(std::string(msg,len));
}
void TcpConnection::send(const std::string &msg){
#if SEND_ORDER
    loop_->runInLoop([=](){
        sendInLoop(msg);
    });
#else
    if(loop_->isInLoopThread())
    {
        sendInLoop(msg);
    }
    else{
        loop_->runInLoop([=](){
            sendInLoop(msg);
        });
    }
#endif
}