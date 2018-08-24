#include <trantor/net/ssl/SSLConnection.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/inner/Channel.h>
#include <trantor/net/inner/Socket.h>
#include <openssl/err.h>
using namespace trantor;
SSLConnection::SSLConnection(EventLoop *loop,int socketfd,const InetAddress& localAddr,
                             const InetAddress& peerAddr,
                             const std::shared_ptr<SSL_CTX> &ctxPtr,
                             bool isServer):
TcpConnectionImpl(loop,socketfd,localAddr,peerAddr),
_sslPtr(std::shared_ptr<SSL>(SSL_new(ctxPtr.get()),[](SSL *ssl){SSL_free(ssl);})),
_isServer(isServer)
{
    assert(_sslPtr);
    auto r=SSL_set_fd(_sslPtr.get(),socketfd);
    assert(r);
}

void SSLConnection::readCallback() {
    LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    if(_status==SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if(_status==SSLStatus::Connected)
    {
        int rd;
        char buf[65536];
        bool newDataFlag=false;
        do{

            rd = SSL_read(_sslPtr.get(), buf, sizeof(buf));
            LOG_TRACE<<"ssl read:"<<rd<<" bytes";
            int sslerr = SSL_get_error(_sslPtr.get(), rd);
            if (rd <= 0 && sslerr != SSL_ERROR_WANT_READ) {
                LOG_ERROR<<"ssl read err:"<<sslerr;
                _status=SSLStatus::DisConnected;
                handleClose();
                return;
            }
            if(rd>0)
            {
                readBuffer_.append(buf,rd);
                newDataFlag=true;
            }

        }while(rd==sizeof(buf));
        if(newDataFlag)
        {
            //eval callback function;
            recvMsgCallback_(shared_from_this(),&readBuffer_);
        }
    }
}
void SSLConnection::writeCallback() {
    LOG_TRACE<<"write Callback";
    loop_->assertInLoopThread();
    if(_status==SSLStatus::Handshaking)
    {
        doHandshaking();
        return;
    }
    else if(_status==SSLStatus::Connected)
    {
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
                int wd = SSL_write(_sslPtr.get(), writeBuffer_.peek(),writeBuffer_.readableBytes());
                int sslerr = SSL_get_error(_sslPtr.get(), wd);
                if (wd <= 0 && sslerr != SSL_ERROR_WANT_WRITE)
                {
                    LOG_ERROR<<"ssl write error:"<<sslerr;
                    ioChennelPtr_->disableReading();
                    _status=SSLStatus::DisConnected;
                    return;
                }
                if(wd>0)
                    writeBuffer_.retrieve(wd);
            }
        }
        else
        {
            LOG_SYSERR<<"no writing but call write callback";
        }

    }
}

void SSLConnection::connectEstablished()
{
    loop_->runInLoop([=](){
        LOG_TRACE<<"connectEstablished";
        assert(state_==Connecting);
        ioChennelPtr_->tie(shared_from_this());
        ioChennelPtr_->enableReading();
        state_=Connected;
        if(_isServer)
        {
            SSL_set_accept_state(_sslPtr.get());
        }
        else{
            ioChennelPtr_->enableWriting();
            SSL_set_connect_state(_sslPtr.get());
        }
    });
}
void SSLConnection::doHandshaking() {
    assert(_status==SSLStatus::Handshaking);
    int r = SSL_do_handshake(_sslPtr.get());
    if (r == 1) {
        _status = SSLStatus::Connected;
        connectionCallback_(shared_from_this());
        return;
    }
    int err = SSL_get_error(_sslPtr.get(), r);
    if (err == SSL_ERROR_WANT_WRITE) { //SSL want writable;
        ioChennelPtr_->enableWriting();
        ioChennelPtr_->disableReading();
    } else if (err == SSL_ERROR_WANT_READ) { //SSL want readable;
        ioChennelPtr_->enableReading();
        ioChennelPtr_->disableWriting();
    } else { //错误
        //ERR_print_errors(err);
        LOG_FATAL<<"SSL handshake err";
        ioChennelPtr_->disableReading();
        _status=SSLStatus::DisConnected;
    }
}

void SSLConnection::sendInLoop(const char *buffer,size_t length)
{
    LOG_TRACE<<"send in loop";
    loop_->assertInLoopThread();
    if(state_!=Connected)
    {
        LOG_WARN<<"Connection is not connected,give up sending";
        return;
    }
    if(_status!=SSLStatus::Connected)
    {
        LOG_WARN<<"SSL is not connected,give up sending";
        return;
    }
    size_t remainLen=length;
    ssize_t sendLen=0;
    if(!ioChennelPtr_->isWriting()&&writeBuffer_.readableBytes()==0)
    {
        //send directly
        sendLen = SSL_write(_sslPtr.get(), buffer,length);
        int sslerr = SSL_get_error(_sslPtr.get(), sendLen);
        if (sendLen < 0 && sslerr != SSL_ERROR_WANT_WRITE)
        {
            LOG_ERROR<<"ssl write error:"<<sslerr;
            return;
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



