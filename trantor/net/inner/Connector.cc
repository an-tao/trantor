#include "Connector.h"
#include "Channel.h"
#include "Socket.h"
using namespace trantor;

Connector::Connector(EventLoop *loop, const InetAddress &addr, bool retry)
    : loop_(loop),
      serverAddr_(addr),
      connect_(false),
      state_(kDisconnected),
      retryInterval_(kInitRetryDelayMs),
      maxRetryInterval_(kMaxRetryDelayMs),
      _retry(retry)
{
}
Connector::Connector(EventLoop *loop, InetAddress &&addr, bool retry)
    : loop_(loop),
      serverAddr_(std::move(addr)),
      connect_(false),
      state_(kDisconnected),
      retryInterval_(kInitRetryDelayMs),
      maxRetryInterval_(kMaxRetryDelayMs),
      _retry(retry)
{
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop([=]() {
        startInLoop();
    });
}
void Connector::restart()
{
}
void Connector::stop()
{
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}
void Connector::connect()
{
    int sockfd = Socket::createNonblockingSocketOrDie(serverAddr_.family());
    int ret = Socket::connect(sockfd, serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        LOG_TRACE << "connecting";
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        break;

    default:
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::connecting(int sockfd)
{
    state_ = kConnecting;
    assert(!channelPtr_);
    channelPtr_.reset(new Channel(loop_, sockfd));
    channelPtr_->setWriteCallback(
        std::bind(&Connector::handleWrite, shared_from_this())); // FIXME: unsafe
    channelPtr_->setErrorCallback(
        std::bind(&Connector::handleError, shared_from_this())); // FIXME: unsafe
    channelPtr_->setCloseCallback(
        std::bind(&Connector::handleError, shared_from_this()));

    channelPtr_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channelPtr_->disableAll();
    channelPtr_->remove();
    int sockfd = channelPtr_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([=]() {
        channelPtr_.reset();
    }); // FIXME: unsafe
    return sockfd;
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            if (_retry)
            {
                retry(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
            if (_errorCallback)
            {
                _errorCallback();
            }
        }
        else if (Socket::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            if (_retry)
            {
                retry(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
            if (_errorCallback)
            {
                _errorCallback();
            }
        }
        else
        {
            state_ = kConnected;
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        state_ = kDisconnected;
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        if (_retry)
        {
            retry(sockfd);
        }
        else
        {
            ::close(sockfd);
        }
        if (_errorCallback)
        {
            _errorCallback();
        }
    }
}

void Connector::retry(int sockfd)
{
    assert(_retry);
    ::close(sockfd);
    state_ = kDisconnected;
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
                 << " in " << retryInterval_ << " milliseconds. ";
        loop_->runAfter(retryInterval_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryInterval_ = std::min(retryInterval_ * 2, maxRetryInterval_);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}
