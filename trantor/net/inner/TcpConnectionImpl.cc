/**
 *
 *  TcpConnectionImpl.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

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
      ioChannelPtr_(new Channel(loop, socketfd)),
      socketPtr_(new Socket(socketfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr)
{
    LOG_TRACE << "new connection:" << peerAddr.toIpPort() << "->"
              << localAddr.toIpPort();
    ioChannelPtr_->setReadCallback(
        std::bind(&TcpConnectionImpl::readCallback, this));
    ioChannelPtr_->setWriteCallback(
        std::bind(&TcpConnectionImpl::writeCallback, this));
    ioChannelPtr_->setCloseCallback(
        std::bind(&TcpConnectionImpl::handleClose, this));
    ioChannelPtr_->setErrorCallback(
        std::bind(&TcpConnectionImpl::handleError, this));
    socketPtr_->setKeepAlive(true);
    name_ = localAddr.toIpPort() + "--" + peerAddr.toIpPort();
}
TcpConnectionImpl::~TcpConnectionImpl()
{
}
void TcpConnectionImpl::readCallback()
{
    // LOG_TRACE<<"read Callback";
    loop_->assertInLoopThread();
    int ret = 0;

    ssize_t n = readBuffer_.readFd(socketPtr_->fd(), &ret);
    // LOG_TRACE<<"read "<<n<<" bytes from socket";
    if (n == 0)
    {
        // socket closed by peer
        handleClose();
    }
    else if (n < 0)
    {
        if (errno == EPIPE || errno == ECONNRESET)  // TODO: any others?
        {
            LOG_DEBUG << "EPIPE or ECONNRESET, erron=" << errno;
            return;
        }
        LOG_SYSERR << "read socket error";
    }
    extendLife();
    if (n > 0)
    {
        bytesReceived_ += n;
        if (recvMsgCallback_)
        {
            recvMsgCallback_(shared_from_this(), &readBuffer_);
        }
    }
}
void TcpConnectionImpl::extendLife()
{
    if (idleTimeout_ > 0)
    {
        auto now = Date::date();
        if (now < lastTimingWheelUpdateTime_.after(1.0))
            return;
        lastTimingWheelUpdateTime_ = now;
        auto entry = kickoffEntry_.lock();
        if (entry)
        {
            timingWheelPtr_->insertEntry(idleTimeout_, entry);
        }
    }
}
void TcpConnectionImpl::writeCallback()
{
    loop_->assertInLoopThread();
    extendLife();
    if (ioChannelPtr_->isWriting())
    {
        assert(!writeBufferList_.empty());
        auto writeBuffer_ = writeBufferList_.front();
        if (writeBuffer_->sendFd_ < 0)
        {
            if (writeBuffer_->msgBuffer_->readableBytes() <= 0)
            {
                writeBufferList_.pop_front();
                if (writeBufferList_.empty())
                {
                    ioChannelPtr_->disableWriting();
                    //
                    if (writeCompleteCallback_)
                        writeCompleteCallback_(shared_from_this());
                    if (status_ == ConnStatus::Disconnecting)
                    {
                        socketPtr_->closeWrite();
                    }
                }
                else
                {
                    auto fileNode = writeBufferList_.front();
                    assert(fileNode->sendFd_ >= 0);
                    sendFileInLoop(fileNode);
                }
            }
            else
            {
                auto n = writeInLoop(writeBuffer_->msgBuffer_->peek(),
                                     writeBuffer_->msgBuffer_->readableBytes());
                if (n >= 0)
                {
                    writeBuffer_->msgBuffer_->retrieve(n);
                }
                else
                {
                    if (errno != EWOULDBLOCK)
                    {
                        // TODO: any others?
                        if (errno == EPIPE || errno == ECONNRESET)
                        {
                            LOG_DEBUG << "EPIPE or ECONNRESET, erron=" << errno;
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
            // file
            if (writeBuffer_->fileBytesToSend_ <= 0)
            {
                writeBufferList_.pop_front();
                if (writeBufferList_.empty())
                {
                    ioChannelPtr_->disableWriting();
                    if (writeCompleteCallback_)
                        writeCompleteCallback_(shared_from_this());
                    if (status_ == ConnStatus::Disconnecting)
                    {
                        socketPtr_->closeWrite();
                    }
                }
                else
                {
                    if (writeBufferList_.front()->sendFd_ < 0)
                    {
                        // There is data to be sent in the buffer.
                        auto n = writeInLoop(
                            writeBufferList_.front()->msgBuffer_->peek(),
                            writeBufferList_.front()
                                ->msgBuffer_->readableBytes());
                        writeBufferList_.front()->msgBuffer_->retrieve(n);
                        if (n >= 0)
                        {
                            writeBufferList_.front()->msgBuffer_->retrieve(n);
                        }
                        else
                        {
                            if (errno != EWOULDBLOCK)
                            {
                                // TODO: any others?
                                if (errno == EPIPE || errno == ECONNRESET)
                                {
                                    LOG_DEBUG << "EPIPE or ECONNRESET, erron="
                                              << errno;
                                    return;
                                }
                                LOG_SYSERR << "Unexpected error(" << errno
                                           << ")";
                                return;
                            }
                        }
                    }
                    else
                    {
                        // more file
                        sendFileInLoop(writeBufferList_.front());
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
    // loop_->assertInLoopThread();
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        LOG_TRACE << "connectEstablished";
        assert(thisPtr->status_ == ConnStatus::Connecting);
        thisPtr->ioChannelPtr_->tie(thisPtr);
        thisPtr->ioChannelPtr_->enableReading();
        thisPtr->status_ = ConnStatus::Connected;
        if (thisPtr->connectionCallback_)
            thisPtr->connectionCallback_(thisPtr);
    });
}
void TcpConnectionImpl::handleClose()
{
    LOG_TRACE << "connection closed";
    loop_->assertInLoopThread();
    status_ = ConnStatus::Disconnected;
    ioChannelPtr_->disableAll();
    //  ioChannelPtr_->remove();
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
    if (err == 0)
        return;
    if (err == EPIPE || err == ECONNRESET || err == 104)
    {
        LOG_DEBUG << "[" << name_ << "] - SO_ERROR = " << err << " "
                  << strerror_tl(err);
    }
    else
    {
        LOG_ERROR << "[" << name_ << "] - SO_ERROR = " << err << " "
                  << strerror_tl(err);
    }
}
void TcpConnectionImpl::setTcpNoDelay(bool on)
{
    socketPtr_->setTcpNoDelay(on);
}
void TcpConnectionImpl::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (status_ == ConnStatus::Connected)
    {
        status_ = ConnStatus::Disconnected;
        ioChannelPtr_->disableAll();

        connectionCallback_(shared_from_this());
    }
    ioChannelPtr_->remove();
}
void TcpConnectionImpl::shutdown()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        if (thisPtr->status_ == ConnStatus::Connected)
        {
            thisPtr->status_ = ConnStatus::Disconnecting;
            if (!thisPtr->ioChannelPtr_->isWriting())
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
        if (thisPtr->status_ == ConnStatus::Connected ||
            thisPtr->status_ == ConnStatus::Disconnecting)
        {
            thisPtr->status_ = ConnStatus::Disconnecting;
            thisPtr->handleClose();
        }
    });
}

void TcpConnectionImpl::sendInLoop(const char *buffer, size_t length)
{
    loop_->assertInLoopThread();
    if (status_ != ConnStatus::Connected)
    {
        LOG_WARN << "Connection is not connected,give up sending";
        return;
    }
    extendLife();
    size_t remainLen = length;
    ssize_t sendLen = 0;
    if (!ioChannelPtr_->isWriting() && writeBufferList_.empty())
    {
        // send directly
        sendLen = writeInLoop(buffer, length);
        if (sendLen < 0)
        {
            // error
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET)  // TODO: any others?
                {
                    LOG_DEBUG << "EPIPE or ECONNRESET, erron=" << errno;
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
        if (writeBufferList_.empty())
        {
            BufferNodePtr node(new BufferNode);
            node->msgBuffer_ = std::shared_ptr<MsgBuffer>(new MsgBuffer);
            writeBufferList_.push_back(std::move(node));
        }
        else if (writeBufferList_.back()->sendFd_ >= 0)
        {
            BufferNodePtr node(new BufferNode);
            node->msgBuffer_ = std::shared_ptr<MsgBuffer>(new MsgBuffer);
            writeBufferList_.push_back(std::move(node));
        }
        writeBufferList_.back()->msgBuffer_->append(buffer + sendLen,
                                                    remainLen);
        if (!ioChannelPtr_->isWriting())
            ioChannelPtr_->enableWriting();
        if (highWaterMarkCallback_ &&
            writeBufferList_.back()->msgBuffer_->readableBytes() >
                highWaterMarkLen_)
        {
            highWaterMarkCallback_(
                shared_from_this(),
                writeBufferList_.back()->msgBuffer_->readableBytes());
        }
    }
}
// The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const std::shared_ptr<std::string> &msgPtr)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(msgPtr->data(), msgPtr->length());
        }
        else
        {
            sendNum_++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msgPtr]() {
                thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, msgPtr]() {
            thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}
void TcpConnectionImpl::send(const char *msg, uint64_t len)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(msg, len);
        }
        else
        {
            sendNum_++;
            auto buffer = std::make_shared<std::string>(msg, len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg, len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}
void TcpConnectionImpl::send(const std::string &msg)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            sendNum_++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msg]() {
                thisPtr->sendInLoop(msg.data(), msg.length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, msg]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}
void TcpConnectionImpl::send(std::string &&msg)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(msg.data(), msg.length());
        }
        else
        {
            auto thisPtr = shared_from_this();
            sendNum_++;
            loop_->queueInLoop([thisPtr, msg = std::move(msg)]() {
                thisPtr->sendInLoop(msg.data(), msg.length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, msg = std::move(msg)]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            sendNum_++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(buffer.peek(), buffer.readableBytes());
        }
        else
        {
            sendNum_++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            thisPtr->sendNum_--;
        });
    }
}
void TcpConnectionImpl::sendFile(const char *fileName,
                                 size_t offset,
                                 size_t length)
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
    node->sendFd_ = sfd;
    node->offset_ = offset;
    node->fileBytesToSend_ = length;
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            writeBufferList_.push_back(node);
            if (writeBufferList_.size() == 1)
            {
                sendFileInLoop(writeBufferList_.front());
                return;
            }
        }
        else
        {
            sendNum_++;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, node]() {
                thisPtr->writeBufferList_.push_back(node);
                {
                    std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                    thisPtr->sendNum_--;
                }

                if (thisPtr->writeBufferList_.size() == 1)
                {
                    thisPtr->sendFileInLoop(thisPtr->writeBufferList_.front());
                }
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        sendNum_++;
        loop_->queueInLoop([thisPtr, node]() {
            LOG_TRACE << "Push sendfile to list";
            thisPtr->writeBufferList_.push_back(node);

            {
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                thisPtr->sendNum_--;
            }

            if (thisPtr->writeBufferList_.size() == 1)
            {
                thisPtr->sendFileInLoop(thisPtr->writeBufferList_.front());
            }
        });
    }
}

void TcpConnectionImpl::sendFileInLoop(const BufferNodePtr &filePtr)
{
    loop_->assertInLoopThread();
    assert(filePtr->sendFd_ >= 0);
#ifdef __linux__
    if (!isSSLConn_)
    {
        auto bytesSent = sendfile(socketPtr_->fd(),
                                  filePtr->sendFd_,
                                  &filePtr->offset_,
                                  filePtr->fileBytesToSend_);
        if (bytesSent < 0)
        {
            if (errno != EAGAIN)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                if (ioChannelPtr_->isWriting())
                    ioChannelPtr_->disableWriting();
            }
            return;
        }
        if (bytesSent < filePtr->fileBytesToSend_)
        {
            if (bytesSent == 0)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                return;
            }
        }
        LOG_TRACE << "sendfile() " << bytesSent << " bytes sent";
        filePtr->fileBytesToSend_ -= bytesSent;
        if (!ioChannelPtr_->isWriting())
        {
            ioChannelPtr_->enableWriting();
        }
        return;
    }
#endif
    lseek(filePtr->sendFd_, filePtr->offset_, SEEK_SET);
    if (!fileBufferPtr_)
    {
        fileBufferPtr_ = std::make_unique<std::vector<char>>(16 * 1024);
    }
    while (filePtr->fileBytesToSend_ > 0)
    {
        auto n = read(filePtr->sendFd_,
                      &(*fileBufferPtr_)[0],
                      fileBufferPtr_->size());
        if (n > 0)
        {
            auto nSend = writeInLoop(&(*fileBufferPtr_)[0], n);
            if (nSend >= 0)
            {
                filePtr->fileBytesToSend_ -= nSend;
                filePtr->offset_ += nSend;
                if (nSend < n)
                {
                    if (!ioChannelPtr_->isWriting())
                    {
                        ioChannelPtr_->enableWriting();
                    }
                    return;
                }
                else if (nSend == n)
                    continue;
            }
            if (nSend < 0)
            {
                if (errno != EWOULDBLOCK)
                {
                    // TODO: any others?
                    if (errno == EPIPE || errno == ECONNRESET)
                    {
                        LOG_DEBUG << "EPIPE or ECONNRESET, erron=" << errno;
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
            if (ioChannelPtr_->isWriting())
                ioChannelPtr_->disableWriting();
            return;
        }
        if (n == 0)
        {
            LOG_SYSERR << "read";
            break;
        }
    }
    if (!ioChannelPtr_->isWriting())
    {
        ioChannelPtr_->enableWriting();
    }
}

ssize_t TcpConnectionImpl::writeInLoop(const char *buffer, size_t length)
{
    bytesSent_ += length;
    return write(socketPtr_->fd(), buffer, length);
}
