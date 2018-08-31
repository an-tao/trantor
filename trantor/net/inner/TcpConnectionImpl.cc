#include "TcpConnectionImpl.h"
#include "Socket.h"
#include "Channel.h"
#define FETCH_SIZE 2048;
using namespace trantor;

void trantor::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddr().toIpPort() << " -> "
              << conn->peerAddr().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void trantor::defaultMessageCallback(const TcpConnectionPtr&,
                                        MsgBuffer* buf)
{
    buf->retrieveAll();
}

TcpConnectionImpl::TcpConnectionImpl(EventLoop *loop, int socketfd,const InetAddress& localAddr,
                             const InetAddress& peerAddr):
        loop_(loop),
        ioChennelPtr_(new Channel(loop,socketfd)),
        socketPtr_(new Socket(socketfd)),
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        state_(Connecting)
{
    LOG_TRACE<<"new connection:"<<peerAddr.toIpPort()<<"->"<<localAddr.toIpPort();
    ioChennelPtr_->setReadCallback(std::bind(&TcpConnectionImpl::readCallback,this));
    ioChennelPtr_->setWriteCallback(std::bind(&TcpConnectionImpl::writeCallback,this));
    ioChennelPtr_->setCloseCallback(
            std::bind(&TcpConnectionImpl::handleClose, this));
    ioChennelPtr_->setErrorCallback(
            std::bind(&TcpConnectionImpl::handleError, this));
    socketPtr_->setKeepAlive(true);
    name_=localAddr.toIpPort()+"--"+peerAddr.toIpPort();
}
TcpConnectionImpl::~TcpConnectionImpl() {

}
void TcpConnectionImpl::readCallback() {
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
void TcpConnectionImpl::writeCallback() {
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
void TcpConnectionImpl::connectEstablished() {
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
void TcpConnectionImpl::handleClose() {
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
void TcpConnectionImpl::handleError() {
    int err = socketPtr_->getSocketError();
    LOG_ERROR << "TcpConnectionImpl::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
void TcpConnectionImpl::setTcpNoDelay(bool on)
{
    socketPtr_->setTcpNoDelay(on);
}
void TcpConnectionImpl::connectDestroyed()
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
void TcpConnectionImpl::shutdown() {
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

void TcpConnectionImpl::forceClose()
{
    loop_->runInLoop([=](){
        if (state_ == Connected || state_ == Disconnecting)
        {
            state_=Disconnecting;
            handleClose();
        }
    });
}

void TcpConnectionImpl::sendInLoop(const char *buffer,size_t length)
{
    loop_->assertInLoopThread();
    if(state_!=Connected)
    {
        LOG_WARN<<"Connection is not connected,give up sending";
        return;
    }
    size_t remainLen=length;
    ssize_t sendLen=0;
    if(!ioChennelPtr_->isWriting()&&writeBuffer_.readableBytes()==0)
    {
        //send directly
        sendLen=write(socketPtr_->fd(),buffer,length);
        if(sendLen<0)
        {
            //error
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
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
        writeBuffer_.append(buffer+sendLen,remainLen);
        if(!ioChennelPtr_->isWriting())
            ioChennelPtr_->enableWriting();
        if(highWaterMarkCallback_&&writeBuffer_.readableBytes()>highWaterMarkLen_)
        {
            highWaterMarkCallback_(shared_from_this(),writeBuffer_.readableBytes());
        }
    }
}

void TcpConnectionImpl::send(const char *msg,uint64_t len){

    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if(_sendNum==0)
        {
            sendInLoop(msg,len);
        }
        else
        {
            _sendNum++;
            auto buffer=std::make_shared<std::string>(msg,len);
            loop_->queueInLoop([=](){
                sendInLoop(buffer->data(),buffer->length());
                std::lock_guard<std::mutex> guard1(_sendNumMutex);
                _sendNum--;
            });
        }
    }
    else{
        auto buffer=std::make_shared<std::string>(msg,len);
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([=](){
            sendInLoop(buffer->data(),buffer->length());
            std::lock_guard<std::mutex> guard1(_sendNumMutex);
            _sendNum--;
        });
    }

}
void TcpConnectionImpl::send(const std::string &msg){
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if(_sendNum==0)
        {
            sendInLoop(msg.data(),msg.length());
        }
        else
        {
            _sendNum++;
            loop_->queueInLoop([=](){
                sendInLoop(msg.data(),msg.length());
                std::lock_guard<std::mutex> guard1(_sendNumMutex);
                _sendNum--;
            });
        }
    }
    else{
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([=](){
            sendInLoop(msg.data(),msg.length());
            std::lock_guard<std::mutex> guard1(_sendNumMutex);
            _sendNum--;
        });
    }
}
void TcpConnectionImpl::send(std::string &&msg){
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if(_sendNum==0)
        {
            sendInLoop(msg.data(),msg.length());
        }
        else
        {
            _sendNum++;
            std::shared_ptr<std::string> msgPtr=
                    std::make_shared<std::string>(std::move(msg));
            loop_->queueInLoop([=](){
                sendInLoop(msgPtr->data(),msgPtr->length());
                std::lock_guard<std::mutex> guard1(_sendNumMutex);
                _sendNum--;
            });
        }
    }
    else{
        std::shared_ptr<std::string> msgPtr=
                std::make_shared<std::string>(std::move(msg));
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([=](){
            sendInLoop(msgPtr->data(),msgPtr->length());
            std::lock_guard<std::mutex> guard1(_sendNumMutex);
            _sendNum--;
        });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer){
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if(_sendNum==0)
        {
            sendInLoop(buffer.peek(),buffer.readableBytes());
        }
        else
        {
            _sendNum++;
            loop_->queueInLoop([=](){
                sendInLoop(buffer.peek(),buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(_sendNumMutex);
                _sendNum--;
            });
        }
    }
    else{
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([=](){
            sendInLoop(buffer.peek(),buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(_sendNumMutex);
            _sendNum--;
        });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer){
    if(loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if(_sendNum==0)
        {
            sendInLoop(buffer.peek(),buffer.readableBytes());
        }
        else
        {
            _sendNum++;
            auto bufferPtr=std::make_shared<MsgBuffer>(std::move(buffer));
            loop_->queueInLoop([=](){
                sendInLoop(bufferPtr->peek(),bufferPtr->readableBytes());
                std::lock_guard<std::mutex> guard1(_sendNumMutex);
                _sendNum--;
            });
        }
    }
    else{
        auto bufferPtr=std::make_shared<MsgBuffer>(std::move(buffer));
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([=](){
            sendInLoop(bufferPtr->peek(),bufferPtr->readableBytes());
            std::lock_guard<std::mutex> guard1(_sendNumMutex);
            _sendNum--;
        });
    }
}