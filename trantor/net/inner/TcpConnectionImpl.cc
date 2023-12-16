/**
 *
 *  @file TcpConnectionImpl.cc
 *  @author An Tao
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
#include <trantor/utils/Utilities.h>
#ifdef __linux__
#include <sys/sendfile.h>
#include <poll.h>
#endif
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <WinSock2.h>
#include <Windows.h>
#include <wincrypt.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

using namespace trantor;

#ifdef _WIN32
// Winsock does not set errno, and WSAGetLastError() has different values than
// errno socket errors
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EPIPE
#define EPIPE WSAENOTCONN
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif

TcpConnectionImpl::TcpConnectionImpl(EventLoop *loop,
                                     int socketfd,
                                     const InetAddress &localAddr,
                                     const InetAddress &peerAddr,
                                     TLSPolicyPtr policy,
                                     SSLContextPtr ctx)
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

    if (policy != nullptr)
    {
        tlsProviderPtr_ = newTLSProvider(this, policy, ctx);
        tlsProviderPtr_->setWriteCallback(onSslWrite);
        tlsProviderPtr_->setErrorCallback(onSslError);
        tlsProviderPtr_->setHandshakeCallback(onHandshakeFinished);
        tlsProviderPtr_->setMessageCallback(onSslMessage);
        // This is triggered when peer sends a close alert
        tlsProviderPtr_->setCloseCallback(onSslCloseAlert);
    }
}
TcpConnectionImpl::~TcpConnectionImpl()
{
    // send a close alert to peer if we are still connected
    if (tlsProviderPtr_ && status_ == ConnStatus::Connected)
        tlsProviderPtr_->close();
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
        if (errno == EPIPE || errno == ECONNRESET)
        {
#ifdef _WIN32
            LOG_TRACE << "WSAENOTCONN or WSAECONNRESET, errno=" << errno
                      << " fd=" << socketPtr_->fd();
#else
            LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno
                      << " fd=" << socketPtr_->fd();
#endif
            return;
        }
#ifdef _WIN32
        if (errno == WSAECONNABORTED)
        {
            LOG_TRACE << "WSAECONNABORTED, errno=" << errno;
            handleClose();
            return;
        }
#else
        if (errno == EAGAIN)  // TODO: any others?
        {
            LOG_TRACE << "EAGAIN, errno=" << errno
                      << " fd=" << socketPtr_->fd();
            return;
        }
#endif
        LOG_SYSERR << "read socket error";
        handleClose();
        return;
    }
    extendLife();
    if (n > 0)
    {
        bytesReceived_ += n;
        if (tlsProviderPtr_)
        {
            tlsProviderPtr_->recvData(&readBuffer_);
        }
        else if (recvMsgCallback_)
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
            auto timingWheelPtr = timingWheelWeakPtr_.lock();
            if (timingWheelPtr)
                timingWheelPtr->insertEntry(idleTimeout_, entry);
        }
    }
}
void TcpConnectionImpl::writeCallback()
{
    loop_->assertInLoopThread();
    extendLife();
    if (ioChannelPtr_->isWriting())
    {
        if (tlsProviderPtr_)
        {
            bool sentAll = tlsProviderPtr_->sendBufferedData();
            if (!sentAll)
            {
                ioChannelPtr_->enableWriting();
                return;
            }
        }
        assert(!writeBufferList_.empty());
        auto nodePtr = writeBufferList_.front();
        // not a file
        if (nodePtr->remainingBytes() == 0)
        {
            // finished sending
            writeBufferList_.pop_front();
            if (writeBufferList_.empty())
            {
                // stop writing
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
                sendNodeInLoop(writeBufferList_.front());
            }
        }
        else
        {
            // continue sending
            sendNodeInLoop(nodePtr);
        }

        if (closeOnEmpty_ &&
            (writeBufferList_.empty() &&
             (tlsProviderPtr_ == nullptr ||
              tlsProviderPtr_->getBufferedData().readableBytes() == 0)))
        {
            shutdown();
        }
    }
    else
    {
        LOG_SYSERR << "no writing but write callback called";
    }
}
void TcpConnectionImpl::connectEstablished()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]() {
        LOG_TRACE << "connectEstablished";
        assert(thisPtr->status_ == ConnStatus::Connecting);
        thisPtr->ioChannelPtr_->tie(thisPtr);
        thisPtr->ioChannelPtr_->enableReading();
        thisPtr->status_ = ConnStatus::Connected;

        if (thisPtr->tlsProviderPtr_)
            thisPtr->tlsProviderPtr_->startEncryption();
        else if (thisPtr->connectionCallback_)
            thisPtr->connectionCallback_(thisPtr);
    });
}
void TcpConnectionImpl::handleClose()
{
    LOG_TRACE << "connection closed, fd=" << socketPtr_->fd();
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
    if (err == EPIPE ||
#ifndef _WIN32
        err == EBADMSG ||  // ??? 104=EBADMSG
#endif
        err == ECONNRESET)
    {
        LOG_TRACE << "[" << name_ << "] - SO_ERROR = " << err << " "
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
            if (thisPtr->tlsProviderPtr_)
            {
                // there's still data to be sent, so we can't close the
                // connection just yet
                if (thisPtr->tlsProviderPtr_->getBufferedData()
                            .readableBytes() != 0 ||
                    thisPtr->writeBufferList_.size() != 0)
                {
                    thisPtr->closeOnEmpty_ = true;
                    return;
                }
                thisPtr->tlsProviderPtr_->close();
            }
            if (thisPtr->tlsProviderPtr_ == nullptr &&
                thisPtr->writeBufferList_.size() != 0)
            {
                thisPtr->closeOnEmpty_ = true;
                return;
            }
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

            if (thisPtr->tlsProviderPtr_)
                thisPtr->tlsProviderPtr_->close();
        }
    });
}
#ifndef _WIN32
void TcpConnectionImpl::sendInLoop(const void *buffer, size_t length)
#else
void TcpConnectionImpl::sendInLoop(const char *buffer, size_t length)
#endif
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
#ifdef _WIN32
            if (errno != 0 && errno != EWOULDBLOCK)
#else
            if (errno != EWOULDBLOCK)
#endif
            {
                if (errno == EPIPE || errno == ECONNRESET)  // TODO: any others?
                {
#ifdef _WIN32
                    LOG_TRACE << "WSAENOTCONN or WSAECONNRESET, errno="
                              << errno;
#else
                    LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno;
#endif
                    return;
                }
                LOG_SYSERR << "Unexpected error(" << errno << ")";
                return;
            }
            sendLen = 0;
        }
        remainLen -= sendLen;
    }
    if (remainLen > 0 && status_ == ConnStatus::Connected)
    {
        if (writeBufferList_.empty() || writeBufferList_.back()->isFile() ||
            writeBufferList_.back()->isStream())
        {
            writeBufferList_.push_back(BufferNode::newMemBufferNode());
        }
        writeBufferList_.back()->append(static_cast<const char *>(buffer) +
                                            sendLen,
                                        remainLen);
        if (!ioChannelPtr_->isWriting())
            ioChannelPtr_->enableWriting();
        if (highWaterMarkCallback_ &&
            writeBufferList_.back()->remainingBytes() > highWaterMarkLen_)
        {
            highWaterMarkCallback_(shared_from_this(),
                                   writeBufferList_.back()->remainingBytes());
        }
        if (highWaterMarkCallback_ && tlsProviderPtr_ &&
            tlsProviderPtr_->getBufferedData().readableBytes() >
                highWaterMarkLen_)
        {
            highWaterMarkCallback_(
                shared_from_this(),
                tlsProviderPtr_->getBufferedData().readableBytes());
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
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msgPtr]() {
                thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, msgPtr]() {
            thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}
// The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const std::shared_ptr<MsgBuffer> &msgPtr)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            sendInLoop(msgPtr->peek(), msgPtr->readableBytes());
        }
        else
        {
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msgPtr]() {
                thisPtr->sendInLoop(msgPtr->peek(), msgPtr->readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, msgPtr]() {
            thisPtr->sendInLoop(msgPtr->peek(), msgPtr->readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}
void TcpConnectionImpl::send(const char *msg, size_t len)
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
            ++sendNum_;
            auto buffer = std::make_shared<std::string>(msg, len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg, len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}
void TcpConnectionImpl::send(const void *msg, size_t len)
{
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
#ifndef _WIN32
            sendInLoop(msg, len);
#else
            sendInLoop(static_cast<const char *>(msg), len);
#endif
        }
        else
        {
            ++sendNum_;
            auto buffer =
                std::make_shared<std::string>(static_cast<const char *>(msg),
                                              len);
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto buffer =
            std::make_shared<std::string>(static_cast<const char *>(msg), len);
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
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
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, msg]() {
                thisPtr->sendInLoop(msg.data(), msg.length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, msg]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
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
            ++sendNum_;
            loop_->queueInLoop([thisPtr, msg = std::move(msg)]() {
                thisPtr->sendInLoop(msg.data(), msg.length());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, msg = std::move(msg)]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
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
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer]() {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
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
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
            --thisPtr->sendNum_;
        });
    }
}
void TcpConnectionImpl::sendFile(const char *fileName,
                                 size_t offset,
                                 size_t length)
{
    assert(fileName);
#ifdef _WIN32
    sendFile(utils::toNativePath(fileName).c_str(), offset, length);
#else   // _WIN32
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
#endif  // _WIN32
}

void TcpConnectionImpl::sendFile(const wchar_t *fileName,
                                 size_t offset,
                                 size_t length)
{
    assert(fileName);
#ifndef _WIN32
    sendFile(utils::toNativePath(fileName).c_str(), offset, length);
#else  // _WIN32
    FILE *fp;
#ifndef _MSC_VER
    fp = _wfopen(fileName, L"rb");
#else   // _MSC_VER
    if (_wfopen_s(&fp, fileName, L"rb") != 0)
        fp = nullptr;
#endif  // _MSC_VER
    if (fp == nullptr)
    {
        LOG_SYSERR << fileName << " open error";
        return;
    }

    if (length == 0)
    {
        struct _stati64 filestat;
        if (_wstati64(fileName, &filestat) < 0)
        {
            LOG_SYSERR << fileName << " stat error";
            fclose(fp);
            return;
        }
        length = filestat.st_size;
    }

    sendFile(fp, offset, length);
#endif  // _WIN32
}

#ifndef _WIN32
void TcpConnectionImpl::sendFile(int sfd, size_t offset, size_t length)
#else
void TcpConnectionImpl::sendFile(FILE *fp, size_t offset, size_t length)
#endif
{
    assert(length > 0);
#ifndef _WIN32
    assert(sfd >= 0);
    auto node = BufferNode::newFileBufferNode(sfd, offset, length);
#else
    assert(fp);
    auto node = BufferNode::newFileBufferNode(fp, offset, length);
#endif
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            writeBufferList_.push_back(node);
            if (writeBufferList_.size() == 1)
            {
                sendNodeInLoop(writeBufferList_.front());
                return;
            }
        }
        else
        {
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, node]() {
                thisPtr->writeBufferList_.push_back(node);
                {
                    std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                    --thisPtr->sendNum_;
                }

                if (thisPtr->writeBufferList_.size() == 1)
                {
                    thisPtr->sendNodeInLoop(thisPtr->writeBufferList_.front());
                }
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, node]() {
            LOG_TRACE << "Push sendfile to list";
            thisPtr->writeBufferList_.push_back(node);

            {
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            }

            if (thisPtr->writeBufferList_.size() == 1)
            {
                thisPtr->sendNodeInLoop(thisPtr->writeBufferList_.front());
            }
        });
    }
}

void TcpConnectionImpl::sendStream(
    std::function<std::size_t(char *, std::size_t)> callback)
{
    auto node = BufferNode::newStreamBufferNode(std::move(callback));
    if (loop_->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        if (sendNum_ == 0)
        {
            writeBufferList_.push_back(node);
            if (writeBufferList_.size() == 1)
            {
                sendNodeInLoop(writeBufferList_.front());
                return;
            }
        }
        else
        {
            ++sendNum_;
            auto thisPtr = shared_from_this();
            loop_->queueInLoop([thisPtr, node]() {
                thisPtr->writeBufferList_.push_back(node);
                {
                    std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                    --thisPtr->sendNum_;
                }

                if (thisPtr->writeBufferList_.size() == 1)
                {
                    thisPtr->sendNodeInLoop(thisPtr->writeBufferList_.front());
                }
            });
        }
    }
    else
    {
        auto thisPtr = shared_from_this();
        std::lock_guard<std::mutex> guard(sendNumMutex_);
        ++sendNum_;
        loop_->queueInLoop([thisPtr, node]() {
            LOG_TRACE << "Push sendstream to list";
            thisPtr->writeBufferList_.push_back(node);

            {
                std::lock_guard<std::mutex> guard1(thisPtr->sendNumMutex_);
                --thisPtr->sendNum_;
            }

            if (thisPtr->writeBufferList_.size() == 1)
            {
                thisPtr->sendNodeInLoop(thisPtr->writeBufferList_.front());
            }
        });
    }
}

void TcpConnectionImpl::sendNodeInLoop(const BufferNodePtr &nodePtr)
{
    loop_->assertInLoopThread();
#ifdef __linux__
    if (nodePtr->isFile() && !tlsProviderPtr_)
    {
        LOG_TRACE << "send file in loop using linux kernel sendfile()";
        auto toSend = nodePtr->remainingBytes();
        auto bytesSent =
            sendfile(socketPtr_->fd(), nodePtr->getFd(), nullptr, toSend);
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
        if (static_cast<size_t>(bytesSent) < toSend)
        {
            if (bytesSent == 0)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                return;
            }
        }
        LOG_TRACE << "sendfile() " << bytesSent << " bytes sent";
        nodePtr->retrieve(bytesSent);
        if (!ioChannelPtr_->isWriting())
        {
            ioChannelPtr_->enableWriting();
        }
        return;
    }
#endif
    // Send stream

    LOG_TRACE << "send node in loop";
    const char *data;
    size_t len;
    while ((nodePtr->remainingBytes() > 0))
    {
        // get next chunk
        nodePtr->getData(data, len);
        if (len == 0)
        {
            nodePtr->done();
            break;
        }
        auto nWritten = writeInLoop(data, len);
        if (nWritten >= 0)
        {
            nodePtr->retrieve(nWritten);
            if (static_cast<std::size_t>(nWritten) < len)
            {
                if (!ioChannelPtr_->isWriting())
                    ioChannelPtr_->enableWriting();
                LOG_TRACE << "send stream in loop: return on partial write "
                             "(socket buffer full?)";
                return;
            }
            continue;
        }
        // nWritten < 0
#ifdef _WIN32
        if (errno != 0 && errno != EWOULDBLOCK)
#else
        if (errno != EWOULDBLOCK)
#endif
        {
            if (errno == EPIPE || errno == ECONNRESET)
            {
#ifdef _WIN32
                LOG_TRACE << "WSAENOTCONN or WSAECONNRESET, errno=" << errno;
#else
                LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno;
#endif
                // abort
                LOG_TRACE << "send stream in loop: return on connection closed";
                nodePtr->done();
                return;
            }
            // TODO: any others?
            LOG_SYSERR << "send stream in loop: return on unexpected error("
                       << errno << ")";
            nodePtr->done();
            return;
        }
        // Socket buffer full - return and wait for next call
        LOG_TRACE << "send stream in loop: break on socket buffer full (?)";
        break;
    }
    if (!ioChannelPtr_->isWriting())
        ioChannelPtr_->enableWriting();
    LOG_TRACE << "send stream in loop: return on loop exit";
    return;
}
#ifndef _WIN32
ssize_t TcpConnectionImpl::writeRaw(const void *buffer, size_t length)
#else
ssize_t TcpConnectionImpl::writeRaw(const char *buffer, size_t length)
#endif
{
    // TODO: Abstract this away to support io_uring (and IOCP?)
#ifndef _WIN32
    int nWritten = write(socketPtr_->fd(), buffer, length);
#else
    int nWritten =
        ::send(socketPtr_->fd(), buffer, static_cast<int>(length), 0);
    errno = (nWritten < 0) ? ::WSAGetLastError() : 0;
#endif
    if (nWritten > 0)
        bytesSent_ += nWritten;
    return nWritten;
}

#ifndef _WIN32
ssize_t TcpConnectionImpl::writeInLoop(const void *buffer, size_t length)
#else
ssize_t TcpConnectionImpl::writeInLoop(const char *buffer, size_t length)
#endif
{
    if (tlsProviderPtr_)
        return tlsProviderPtr_->sendData((const char *)buffer, length);
    else
        return writeRaw(buffer, length);
}

#if !(defined(USE_OPENSSL) || defined(USE_BOTAN))
SSLContextPtr trantor::newSSLContext(const TLSPolicy &policy, bool isServer)
{
    (void)policy;
    (void)isServer;
    throw std::runtime_error("SSL is not supported");
}

std::shared_ptr<TLSProvider> trantor::newTLSProvider(TcpConnection *conn,
                                                     TLSPolicyPtr policy,
                                                     SSLContextPtr sslContext)
{
    (void)conn;
    (void)policy;
    (void)sslContext;
    throw std::runtime_error("SSL is not supported");
}
#endif

void TcpConnectionImpl::startEncryption(
    TLSPolicyPtr policy,
    bool isServer,
    std::function<void(const TcpConnectionPtr &)> upgradeCallback)
{
    if (tlsProviderPtr_ || upgradeCallback_)
    {
        LOG_ERROR << "TLS is already started";
        return;
    }
    auto sslContextPtr = newSSLContext(*policy, isServer);
    tlsProviderPtr_ =
        newTLSProvider(this, std::move(policy), std::move(sslContextPtr));
    tlsProviderPtr_->setWriteCallback(onSslWrite);
    tlsProviderPtr_->setErrorCallback(onSslError);
    tlsProviderPtr_->setHandshakeCallback(onHandshakeFinished);
    tlsProviderPtr_->setMessageCallback(onSslMessage);
    // This is triggered when peer sends a close alert
    tlsProviderPtr_->setCloseCallback(onSslCloseAlert);
    tlsProviderPtr_->startEncryption();
    upgradeCallback_ = std::move(upgradeCallback);
}

void TcpConnectionImpl::onSslError(TcpConnection *self, SSLError err)
{
    if (self->sslErrorCallback_)
        self->sslErrorCallback_(err);
    self->forceClose();
}
void TcpConnectionImpl::onHandshakeFinished(TcpConnection *self)
{
    auto connPtr = ((TcpConnectionImpl *)self)->shared_from_this();
    if (connPtr->upgradeCallback_)
    {
        connPtr->upgradeCallback_(connPtr);
        connPtr->upgradeCallback_ = nullptr;
    }
    else if (self->connectionCallback_)
        self->connectionCallback_(connPtr);
}
void TcpConnectionImpl::onSslMessage(TcpConnection *self, MsgBuffer *buffer)
{
    if (self->recvMsgCallback_)
        self->recvMsgCallback_(((TcpConnectionImpl *)self)->shared_from_this(),
                               buffer);
}
ssize_t TcpConnectionImpl::onSslWrite(TcpConnection *self,
                                      const void *data,
                                      size_t len)
{
    auto connPtr = (TcpConnectionImpl *)self;
    return connPtr->writeRaw((const char *)data, len);
}
void TcpConnectionImpl::onSslCloseAlert(TcpConnection *self)
{
    self->shutdown();
}
