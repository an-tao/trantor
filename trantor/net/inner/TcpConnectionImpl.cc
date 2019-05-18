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
    : _loop(loop),
      _ioChannelPtr(new Channel(loop, socketfd)),
      _socketPtr(new Socket(socketfd)),
      _localAddr(localAddr),
      _peerAddr(peerAddr),
      _state(Connecting)
{
    LOG_TRACE << "new connection:" << peerAddr.toIpPort() << "->"
              << localAddr.toIpPort();
    _ioChannelPtr->setReadCallback(
        std::bind(&TcpConnectionImpl::readCallback, this));
    _ioChannelPtr->setWriteCallback(
        std::bind(&TcpConnectionImpl::writeCallback, this));
    _ioChannelPtr->setCloseCallback(
        std::bind(&TcpConnectionImpl::handleClose, this));
    _ioChannelPtr->setErrorCallback(
        std::bind(&TcpConnectionImpl::handleError, this));
    _socketPtr->setKeepAlive(true);
    _name = localAddr.toIpPort() + "--" + peerAddr.toIpPort();
}
TcpConnectionImpl::~TcpConnectionImpl()
{
}
void TcpConnectionImpl::readCallback()
{
    // LOG_TRACE<<"read Callback";
    _loop->assertInLoopThread();
    int ret = 0;

    ssize_t n = _readBuffer.readFd(_socketPtr->fd(), &ret);
    // LOG_TRACE<<"read "<<n<<" bytes from socket";
    if (n == 0)
    {
        // socket closed by peer
        handleClose();
    }
    else if (n < 0)
    {
        LOG_SYSERR << "read socket error";
    }
    extendLife();
    if (n > 0 && _recvMsgCallback)
    {
        _recvMsgCallback(shared_from_this(), &_readBuffer);
    }
}
void TcpConnectionImpl::extendLife()
{
    if (_idleTimeout > 0)
    {
        auto now = Date::date();
        if (now < _lastTimingWheelUpdateTime.after(1.0))
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
    _loop->assertInLoopThread();
    extendLife();
    if (_ioChannelPtr->isWriting())
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
                    _ioChannelPtr->disableWriting();
                    //
                    if (_writeCompleteCallback)
                        _writeCompleteCallback(shared_from_this());
                    if (_state == Disconnecting)
                    {
                        _socketPtr->closeWrite();
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
                    // error
                    if (errno != EWOULDBLOCK)
                    {
                        LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                        if (errno == EPIPE ||
                            errno == ECONNRESET)  // TODO: any others?
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
            // file
            if (writeBuffer_->_fileBytesToSend <= 0)
            {
                _writeBufferList.pop_front();
                if (_writeBufferList.empty())
                {
                    _ioChannelPtr->disableWriting();
                    if (_writeCompleteCallback)
                        _writeCompleteCallback(shared_from_this());
                    if (_state == Disconnecting)
                    {
                        _socketPtr->closeWrite();
                    }
                }
                else
                {
                    if (_writeBufferList.front()->_sendFd < 0)
                    {
                        // There is data to be sent in the buffer.
                        auto n = writeInLoop(_writeBufferList.front()
                                                 ->_msgBuffer->peek(),
                                             _writeBufferList.front()
                                                 ->_msgBuffer->readableBytes());
                        _writeBufferList.front()->_msgBuffer->retrieve(n);
                        if (n >= 0)
                        {
                            _writeBufferList.front()->_msgBuffer->retrieve(n);
                        }
                        else
                        {
                            // error
                            if (errno != EWOULDBLOCK)
                            {
                                LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                                if (errno == EPIPE ||
                                    errno == ECONNRESET)  // TODO: any others?
                                {
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
    //_loop->assertInLoopThread();
    auto thisPtr = shared_from_this();
    _loop->runInLoop([thisPtr]() {
        LOG_TRACE << "connectEstablished";
        assert(thisPtr->_state == Connecting);
        thisPtr->_ioChannelPtr->tie(thisPtr);
        thisPtr->_ioChannelPtr->enableReading();
        thisPtr->_state = Connected;
        if (thisPtr->_connectionCallback)
            thisPtr->_connectionCallback(thisPtr);
    });
}
void TcpConnectionImpl::handleClose()
{
    LOG_TRACE << "connection closed";
    _loop->assertInLoopThread();
    _state = Disconnected;
    _ioChannelPtr->disableAll();
    //  _ioChannelPtr->remove();
    auto guardThis = shared_from_this();
    if (_connectionCallback)
        _connectionCallback(guardThis);
    if (_closeCallback)
    {
        LOG_TRACE << "to call close callback";
        _closeCallback(guardThis);
    }
}
void TcpConnectionImpl::handleError()
{
    int err = _socketPtr->getSocketError();
    if (err != 104)
        LOG_ERROR << "TcpConnectionImpl::handleError [" << _name
                  << "] - SO_ERROR = " << err << " " << strerror_tl(err);
    else
        LOG_INFO << "TcpConnectionImpl::handleError [" << _name
                 << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
void TcpConnectionImpl::setTcpNoDelay(bool on)
{
    _socketPtr->setTcpNoDelay(on);
}
void TcpConnectionImpl::connectDestroyed()
{
    _loop->assertInLoopThread();
    if (_state == Connected)
    {
        _state = Disconnected;
        _ioChannelPtr->disableAll();

        _connectionCallback(shared_from_this());
    }
    _ioChannelPtr->remove();
}
void TcpConnectionImpl::shutdown()
{
    auto thisPtr = shared_from_this();
    _loop->runInLoop([thisPtr]() {
        if (thisPtr->_state == Connected)
        {
            thisPtr->_state = Disconnecting;
            if (!thisPtr->_ioChannelPtr->isWriting())
            {
                thisPtr->_socketPtr->closeWrite();
            }
        }
    });
}

void TcpConnectionImpl::forceClose()
{
    auto thisPtr = shared_from_this();
    _loop->runInLoop([thisPtr]() {
        if (thisPtr->_state == Connected || thisPtr->_state == Disconnecting)
        {
            thisPtr->_state = Disconnecting;
            thisPtr->handleClose();
        }
    });
}

void TcpConnectionImpl::sendInLoop(const char *buffer, size_t length)
{
    _loop->assertInLoopThread();
    if (_state != Connected)
    {
        LOG_WARN << "Connection is not connected,give up sending";
        return;
    }
    extendLife();
    size_t remainLen = length;
    ssize_t sendLen = 0;
    if (!_ioChannelPtr->isWriting() && _writeBufferList.empty())
    {
        // send directly
        sendLen = writeInLoop(buffer, length);
        if (sendLen < 0)
        {
            // error
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET)  // TODO: any others?
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
        _writeBufferList.back()->_msgBuffer->append(buffer + sendLen,
                                                    remainLen);
        if (!_ioChannelPtr->isWriting())
            _ioChannelPtr->enableWriting();
        if (_highWaterMarkCallback &&
            _writeBufferList.back()->_msgBuffer->readableBytes() >
                _highWaterMarkLen)
        {
            _highWaterMarkCallback(shared_from_this(),
                                   _writeBufferList.back()
                                       ->_msgBuffer->readableBytes());
        }
    }
}
// The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const std::shared_ptr<std::string> &msgPtr)
{
    if (_loop->isInLoopThread())
    {
        std::lock_guard<std::mutex> guard(_sendNumMutex);
        if (_sendNum == 0)
        {
            sendInLoop(msgPtr->data(), msgPtr->length());
        }
        else
        {
            _sendNum++;
            auto thisPtr = shared_from_this();
            _loop->queueInLoop([thisPtr, msgPtr]() {
                thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
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
        _loop->queueInLoop([thisPtr, msgPtr]() {
            thisPtr->sendInLoop(msgPtr->data(), msgPtr->length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::send(const char *msg, uint64_t len)
{
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, buffer]() {
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
        _loop->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer->data(), buffer->length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::send(const std::string &msg)
{
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, msg]() {
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
        _loop->queueInLoop([thisPtr, msg]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}
void TcpConnectionImpl::send(std::string &&msg)
{
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, msg = std::move(msg)]() {
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
        _loop->queueInLoop([thisPtr, msg = std::move(msg)]() {
            thisPtr->sendInLoop(msg.data(), msg.length());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer)
{
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, buffer]() {
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
        _loop->queueInLoop([thisPtr, buffer]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
        });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer)
{
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
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
        _loop->queueInLoop([thisPtr, buffer = std::move(buffer)]() {
            thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            std::lock_guard<std::mutex> guard1(thisPtr->_sendNumMutex);
            thisPtr->_sendNum--;
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
    node->_sendFd = sfd;
    node->_offset = offset;
    node->_fileBytesToSend = length;
    if (_loop->isInLoopThread())
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
            _loop->queueInLoop([thisPtr, node]() {
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
        _loop->queueInLoop([thisPtr, node]() {
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
    _loop->assertInLoopThread();
    assert(filePtr->_sendFd >= 0);
#ifdef __linux__
    if (!_isSSLConn)
    {
        auto bytesSent = sendfile(_socketPtr->fd(),
                                  filePtr->_sendFd,
                                  &filePtr->_offset,
                                  filePtr->_fileBytesToSend);
        if (bytesSent < 0)
        {
            if (errno != EAGAIN)
            {
                LOG_SYSERR << "TcpConnectionImpl::sendFileInLoop";
                if (_ioChannelPtr->isWriting())
                    _ioChannelPtr->disableWriting();
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
        if (!_ioChannelPtr->isWriting())
        {
            _ioChannelPtr->enableWriting();
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
                    if (!_ioChannelPtr->isWriting())
                    {
                        _ioChannelPtr->enableWriting();
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
                    if (errno == EPIPE ||
                        errno == ECONNRESET)  // TODO: any others?
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
            if (_ioChannelPtr->isWriting())
                _ioChannelPtr->disableWriting();
            return;
        }
        if (n == 0)
        {
            LOG_SYSERR << "read";
            break;
        }
    }
    if (!_ioChannelPtr->isWriting())
    {
        _ioChannelPtr->enableWriting();
    }
}

ssize_t TcpConnectionImpl::writeInLoop(const char *buffer, size_t length)
{
    return write(_socketPtr->fd(), buffer, length);
}
