#include "TcpConnectionImpl.h"
#include "Socket.h"
#include "Channel.h"
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace trantor;

TcpConnectionImpl::TcpConnectionImpl(EventLoop *loop,
                                     int socketfd,
                                     const InetAddress &localAddr,
                                     const InetAddress &peerAddr)
    : loop_(loop),
      ioChennelPtr_(new Channel(loop, socketfd)),
      socketPtr_(new Socket(socketfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      state_(Connecting)
{
    LOG_TRACE << "new connection:" << peerAddr.toIpPort() << "->" << localAddr.toIpPort();
    ioChennelPtr_->setReadCallback(std::bind(&TcpConnectionImpl::readCallback, this));
    ioChennelPtr_->setWriteCallback(std::bind(&TcpConnectionImpl::writeCallback, this));
    ioChennelPtr_->setCloseCallback(
        std::bind(&TcpConnectionImpl::handleClose, this));
    ioChennelPtr_->setErrorCallback(
        std::bind(&TcpConnectionImpl::handleError, this));
    socketPtr_->setKeepAlive(true);
    name_ = localAddr.toIpPort() + "--" + peerAddr.toIpPort();
}
TcpConnectionImpl::~TcpConnectionImpl()
{
}
void TcpConnectionImpl::readCallback()
{
    //LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    int ret = 0;

    ssize_t n = readBuffer_.readFd(socketPtr_->fd(), &ret);
    //LOG_TRACE<<"read "<<n<<" bytes from socket";
    if (n == 0)
    {
        //socket closed by peer
        handleClose();
    }
    else if (n < 0)
    {
        LOG_SYSERR << "read socket error";
    }
    extendLife();
    if (n > 0 && recvMsgCallback_)
    {
        recvMsgCallback_(shared_from_this(), &readBuffer_);
    }
}
void TcpConnectionImpl::extendLife()
{
    if (_idleTimeout > 0)
    {
        auto now = Date::date();
        if(now < _lastTimingWheelUpdateTime.after(1.0))
            return;
        _lastTimingWheelUpdateTime = now;
        auto entry = _kickoffEntry.lock();
        if (entry)
        {
            _timingWheelPtr->insertEntry(_idleTimeout, entry);
        }
    }
}
void TcpConnectionImpl::writeCallback()
{
    loop_->assertInLoopThread();
    extendLife();
    if (ioChennelPtr_->isWriting())
    {
        assert(!_writeBufferList.empty());
        auto writeBuffer_ = _writeBufferList.front();
        if (writeBuffer_->_sendFd < 0)
        {
            if (writeBuffer_->_msgBuffer->readableBytes() <= 0)
            {
                _writeBufferList.pop_front();
                if (_writeBufferList.empty())
                {
                    ioChennelPtr_->disableWriting();
                    //
                    if (writeCompleteCallback_)
                        writeCompleteCallback_(shared_from_this());
                    if (state_ == Disconnecting)
                    {
                        socketPtr_->closeWrite();
                    }
                }
                else
                {
                    auto fileNode = _writeBufferList.front();
                    assert(fileNode->_sendFd >= 0);
                    sendFileInLoop(fileNode);
                }
            }
            else
            {
                auto n = writeInLoop(writeBuffer_->_msgBuffer->peek(),
                                     writeBuffer_->_msgBuffer->readableBytes());
                if (n >= 0)
                {
                    writeBuffer_->_msgBuffer->retrieve(n);
                }
                else
                {
                    //error
                    if (errno != EWOULDBLOCK)
                    {
                        LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                        {
                            return;
                        }
                        LOG_SYSERR << "Unexpected error(" << errno << ")";
                        return;
                    }
                }
            }
        }
        else
        {
            //file
            if (writeBuffer_->_fileBytesToSend <= 0)
            {
                _writeBufferList.pop_front();
                if (_writeBufferList.empty())
                {
                    ioChennelPtr_->disableWriting();
                    if (writeCompleteCallback_)
                        writeCompleteCallback_(shared_from_this());
                    if (state_ == Disconnecting)
                    {
                        socketPtr_->closeWrite();
                    }
                }
                else
                {
                    if (_writeBufferList.front()->_sendFd < 0)
                    {
                        //There is data to be sent in the buffer.
                        auto n = writeInLoop(_writeBufferList.front()->_msgBuffer->peek(),
                                             _writeBufferList.front()->_msgBuffer->readableBytes());
                        _writeBufferList.front()->_msgBuffer->retrieve(n);
                        if (n >= 0)
                        {
                            _writeBufferList.front()->_msgBuffer->retrieve(n);
                        }
                        else
                        {
                            //error
                            if (errno != EWOULDBLOCK)
                            {
                                LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                                {
                                    return;
                                }
                                LOG_SYSERR << "Unexpected error(" << errno << ")";
                                return;
                            }
                        }
                    }
                    else
                    {
                        //more file
                        sendFileInLoop(_writeBufferList.front());
                    }
                }
            }
            else
            {
                sendFileInLoop(writeBuffer_);
            }
        }
    }
    else
    {
        LOG_SYSERR << "no writing but call write callback";
    }
}
void TcpConnectionImpl::connectEstablished()
{
    //loop_->assertInLoopThread();
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        LOG_TRACE << "connectEstablished";
        assert(thisPtr->state_ == Connecting);
        thisPtr->ioChennelPtr_->tie(thisPtr);
        thisPtr->ioChennelPtr_->enableReading();
        thisPtr->state_ = Connected;
        if (thisPtr->connectionCallback_)
            thisPtr->connectionCallback_(thisPtr);
    });
}
void TcpConnectionImpl::handleClose()
{
    LOG_TRACE << "connection closed";
    loop_->assertInLoopThread();
    state_ = Disconnected;
    ioChennelPtr_->disableAll();
    auto guardThis = shared_from_this();
    if (connectionCallback_)
        connectionCallback_(guardThis);
    if (closeCallback_)
    {
        LOG_TRACE << "to call close callback";
        closeCallback_(guardThis);
    }
}
void TcpConnectionImpl::handleError()
{
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
        state_ = Disconnected;
        ioChennelPtr_->disableAll();

        connectionCallback_(shared_from_this());
    }
    ioChennelPtr_->remove();
}
void TcpConnectionImpl::shutdown()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        if (thisPtr->state_ == Connected)
        {
            thisPtr->state_ = Disconnecting;
            if (!thisPtr->ioChennelPtr_->isWriting())
            {
                thisPtr->socketPtr_->closeWrite();
            }
        }
    });
}

void TcpConnectionImpl::forceClose()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        if (thisPtr->state_ == Connected || thisPtr->state_ == Disconnecting)
        {
            thisPtr->state_ = Disconnecting;
            thisPtr->handleClose();
        }
    });
}

void TcpConnectionImpl::sendInLoop(const char *buffer, size_t length)
{
    loop_->assertInLoopThread();
    if (state_ != Connected)
    {
        LOG_WARN << "Connection is not connected,give up sending";
        return;
    }
    extendLife();
    size_t remainLen = length;
    ssize_t sendLen = 0;
    if (!ioChennelPtr_->isWriting() && _writeBufferList.empty())
    {
        //send directly
        sendLen = writeInLoop(buffer, length);
        if (sendLen < 0)
        {
            //error
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    return;
                }
                LOG_SYSERR << "Unexpected error(" << errno << ")";
                return;
            }
            sendLen = 0;
        }
        remainLen -= sendLen;
    }
    if (remainLen > 0)
    {
        if (_writeBufferList.empty())
        {
            BufferNodePtr node(new BufferNode);
            node->_msgBuffer = std::shared_ptr<MsgBuffer>(new MsgBuffer);
            _writeBufferList.push_back(std::move(node));
        }
        else if (_writeBufferList.back()->_sendFd >= 0)
        {
            BufferNodePtr node(new BufferNode);
            node->_msgBuffer = std::shared_ptr<MsgBuffer>(new MsgBuffer);
            _writeBufferList.push_back(std::move(node));
        }
        _writeBufferList.back()->_msgBuffer->append(buffer + sendLen, remainLen);
        if (!ioChennelPtr_->isWriting())
            ioChennelPtr_->enableWriting();
        if (highWaterMarkCallback_ && _writeBufferList.back()->_msgBuffer->readableBytes() > highWaterMarkLen_)
        {
            highWaterMarkCallback_(shared_from_this(),
                                   _writeBufferList.back()->_msgBuffer->readableBytes());
        }
    }
}
//The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const char *msg, uint64_t len)
{

    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(msg, len);
        }
        else
        {
            _sendNum++;
            auto buffer = std::make_shared<std::string>(msg, len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg, len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::send(const std::string &msg)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            _sendNum++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msg]() {
                thisPtr->sendInLoop(msg.data(), msg.length());
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, msg]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::send(std::string &&msg)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            auto thisPtr = shared_from_this();
            _sendNum++;
            std::shared_ptr<std::string> msgPtr =
                std::make_shared<std::string>(std::move(msg));
            loop_->queueInLoop([thisPtr, msgPtr]() {
                thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            });
        }
    }
    else
    {
        std::shared_ptr<std::string> msgPtr =
            std::make_shared<std::string>(std::move(msg));
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, msgPtr]() {
            thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            _sendNum++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            _sendNum++;
            auto thisPtr = shared_from_this();
            auto bufferPtr = std::make_shared<MsgBuffer>(std::move(buffer));
            loop_->queueInLoop([thisPtr, bufferPtr]() {
                thisPtr->sendInLoop(bufferPtr->peek(), bufferPtr->readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        auto bufferPtr = std::make_shared<MsgBuffer>(std::move(buffer));
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, bufferPtr]() {
            thisPtr->sendInLoop(bufferPtr->peek(), bufferPtr->readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::sendFile(const char *fileName, size_t offset, size_t length)
{
    assert(fileName);

    int fd = open(fileName, O_RDONLY);

    if (fd < 0)
    {
        LOG_SYSERR << fileName << " open error";
        return;
    }

    if (length == 0)
    {
        struct stat filestat;
        if (stat(fileName, &filestat) < 0)
        {
            LOG_SYSERR << fileName << " stat error";
            close(fd);
            return;
        }
        length = filestat.st_size;
    }

    sendFile(fd, offset, length);
}
void TcpConnectionImpl::sendFile(int sfd, size_t offset, size_t length)
{
    assert(length > 0);
    assert(sfd >= 0);

    BufferNodePtr node(new BufferNode);
    node->_sendFd = sfd;
    node->_offset = offset;
    node->_fileBytesToSend = length;
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            _writeBufferList.push_back(node);
            if (_writeBufferList.size() == 1)
            {
                sendFileInLoop(_writeBufferList.front());
                return;
            }
        }
        else
        {
            _sendNum++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, node]() {
                thisPtr->_writeBufferList.push_back(node);
                {
                    std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                    thisPtr->_sendNum--;
                }

                if (thisPtr->_writeBufferList.size() == 1)
                {
                    thisPtr->sendFileInLoop(thisPtr->_writeBufferList.front());
                }
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        _sendNum++;
        loop_->queueInLoop([thisPtr, node]() {
            LOG_TRACE << "Push sendfile to list";
            thisPtr->_writeBufferList.push_back(node);

            {
                std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
                thisPtr->_sendNum--;
            }

            if (thisPtr->_writeBufferList.size() == 1)
            {
                thisPtr->sendFileInLoop(thisPtr->_writeBufferList.front());
            }
        });
    }
}

void TcpConnectionImpl::sendFileInLoop(const BufferNodePtr &filePtr)
{
    loop_->assertInLoopThread();
    assert(filePtr->_sendFd >= 0);
#ifdef __linux__
    if (!_isSSLConn)
    {
        auto bytesSent = sendfile(socketPtr_->fd(), filePtr->_sendFd, &filePtr->_offset, filePtr->_fileBytesToSend);
        if (bytesSent < 0)
        {
            if (errno != EAGAIN)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                if (ioChennelPtr_->isWriting())
                    ioChennelPtr_->disableWriting();
            }
            return;
        }
        if (bytesSent < filePtr->_fileBytesToSend)
        {
            if (bytesSent == 0)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                return;
            }
        }
        LOG_TRACE << "sendfile() " << bytesSent << " bytes sent";
        filePtr->_fileBytesToSend -= bytesSent;
        if (!ioChennelPtr_->isWriting())
        {
            ioChennelPtr_->enableWriting();
        }
        return;
    }
#endif
    lseek(filePtr->_sendFd, filePtr->_offset, SEEK_SET);
    while (filePtr->_fileBytesToSend > 0)
    {
        std::vector<char> buf(1024 * 1024);
        auto n = read(filePtr->_sendFd, &buf[0], buf.size());
        if (n > 0)
        {
            auto nSend = writeInLoop(&buf[0], n);
            if (nSend > 0)
            {
                filePtr->_fileBytesToSend -= nSend;
                filePtr->_offset += nSend;
                if (nSend < n)
                {
                    if (!ioChennelPtr_->isWriting())
                    {
                        ioChennelPtr_->enableWriting();
                    }
                    return;
                }
                else if (nSend == n)
                    continue;
            }
            if (nSend == 0)
                return;
            if (nSend < 0)
            {
                if (errno != EWOULDBLOCK)
                {
                    LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                    if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                    {
                        return;
                    }
                    LOG_SYSERR << "Unexpected error(" << errno << ")";
                    return;
                }
                return;
            }
        }
        if (n < 0)
        {
            LOG_SYSERR << "read error";
            if (ioChennelPtr_->isWriting())
                ioChennelPtr_->disableWriting();
            return;
        }
        if (n == 0)
        {
            LOG_SYSERR << "read";
            break;
        }
    }
    if (!ioChennelPtr_->isWriting())
    {
        ioChennelPtr_->enableWriting();
    }
}

ssize_t TcpConnectionImpl::writeInLoop(const char *buffer, size_t length)
{
    return write(socketPtr_->fd(), buffer, length);
}
