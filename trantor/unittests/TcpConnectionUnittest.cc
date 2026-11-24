#include <trantor/net/EventLoopThread.h>
#include <trantor/net/TcpClient.h>
#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using namespace trantor;
using namespace std::chrono_literals;

namespace
{
class TcpConnectionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        loopThread_.run();
        loop_ = loopThread_.getLoop();
    }

    void TearDown() override
    {
        if (client_)
            client_->stop();
        if (server_)
            server_->stop();
        std::promise<void> done;
        auto doneFuture = done.get_future();
        loop_->runInLoop([this, &done]() {
            client_.reset();
            server_.reset();
            loop_->queueInLoop([&done]() { done.set_value(); });
        });
        doneFuture.wait();
        loop_->quit();
        loopThread_.wait();
    }

    EventLoopThread loopThread_;
    EventLoop *loop_{nullptr};
    std::shared_ptr<TcpClient> client_;
    std::shared_ptr<TcpServer> server_;
};

#ifdef _WIN32
template <typename Pred>
bool waitFor(Pred pred, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred())
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(10ms);
    }
    return true;
}

class ScopedNativeSocket : NonCopyable
{
  public:
    explicit ScopedNativeSocket(SOCKET socket) : fd(socket)
    {
    }

    ~ScopedNativeSocket()
    {
        if (fd != INVALID_SOCKET)
            ::closesocket(fd);
    }

    SOCKET fd;
};

int waitForReadable(SOCKET socket)
{
    fd_set readers;
    FD_ZERO(&readers);
    FD_SET(socket, &readers);
    timeval timeout{5, 0};
    return ::select(0, &readers, nullptr, nullptr, &timeout);
}

struct LoggerOutputAccess : Logger
{
    using Logger::flushFunc_;
    using Logger::outputFunc_;
};

class ScopedLogCapture : NonCopyable
{
  public:
    explicit ScopedLogCapture(std::string &output,
                              Logger::LogLevel level = Logger::kError)
        : output_(LoggerOutputAccess::outputFunc_()),
          flush_(LoggerOutputAccess::flushFunc_()),
          level_(Logger::logLevel())
    {
        Logger::setLogLevel(level);
        Logger::setOutputFunction(
            [&output](const char *message, const uint64_t length) {
                output.append(message, static_cast<size_t>(length));
            },
            []() {});
    }

    ~ScopedLogCapture()
    {
        Logger::setOutputFunction(std::move(output_), std::move(flush_));
        Logger::setLogLevel(level_);
    }

  private:
    std::function<void(const char *, uint64_t)> output_;
    std::function<void()> flush_;
    Logger::LogLevel level_;
};

struct AbortedSendResult
{
    int readable{-1};
    int received{-1};
    int sendError{0};
    bool connectedBeforeSend{false};
    std::string errors;
    std::string trace;
};
#endif
}  // namespace

#ifdef _WIN32
TEST_F(TcpConnectionTest, AbortedSendDoesNotLogUnexpectedError)
{
    ScopedNativeSocket listener(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    ASSERT_NE(listener.fd, INVALID_SOCKET);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = ::inet_addr("127.0.0.1");
    ASSERT_EQ(::bind(listener.fd,
                     reinterpret_cast<sockaddr *>(&address),
                     sizeof(address)),
              0);
    ASSERT_EQ(::listen(listener.fd, 1), 0);
    int addressLength = sizeof(address);
    ASSERT_EQ(::getsockname(listener.fd,
                            reinterpret_cast<sockaddr *>(&address),
                            &addressLength),
              0);

    auto socket = std::make_shared<int>(-1);
    auto ready = std::make_shared<std::promise<void>>();
    auto completed = std::make_shared<std::promise<AbortedSendResult>>();
    auto readyFuture = ready->get_future();
    auto completedFuture = completed->get_future();
    client_ = std::make_shared<TcpClient>(loop_,
                                          InetAddress("127.0.0.1",
                                                      ntohs(address.sin_port)),
                                          "aborted-send-client");
    client_->setMessageCallback(
        [](const TcpConnectionPtr &, MsgBuffer *buffer) {
            buffer->retrieveAll();
        });
    client_->setSockOptCallback([socket](int fd) { *socket = fd; });
    client_->setConnectionCallback([socket, ready, completed](
                                       const TcpConnectionPtr &connection) {
        if (!connection->connected())
            return;
        ready->set_value();

        // Consume EOF before Trantor's read callback closes the connection.
        AbortedSendResult result;
        result.readable = waitForReadable(static_cast<SOCKET>(*socket));
        if (result.readable == 1)
        {
            char byte;
            result.received = ::recv(*socket, &byte, 1, 0);
            if (result.received == 0)
            {
                // Writing after the peer's graceful close provokes an
                // abort. Wait for the actual Winsock error, not a delay.
                waitFor(
                    [&result, socket]() {
                        if (::send(*socket, "x", 1, 0) != SOCKET_ERROR)
                            return false;
                        result.sendError = ::WSAGetLastError();
                        return result.sendError != WSAEWOULDBLOCK;
                    },
                    3s);
                if (result.sendError == WSAECONNABORTED)
                {
                    result.connectedBeforeSend = connection->connected();
                    {
                        ScopedLogCapture capture(result.errors);
                        connection->send("x", 1);
                    }
#ifndef NDEBUG
                    {
                        ScopedLogCapture capture(result.trace, Logger::kTrace);
                        connection->send("x", 1);
                    }
#endif
                }
            }
        }
        completed->set_value(std::move(result));
    });

    client_->connect();
    ASSERT_EQ(waitForReadable(listener.fd), 1);
    ScopedNativeSocket peer(::accept(listener.fd, nullptr, nullptr));
    ASSERT_NE(peer.fd, INVALID_SOCKET);
    ASSERT_EQ(readyFuture.wait_for(5s), std::future_status::ready);
    ASSERT_EQ(::closesocket(peer.fd), 0);
    peer.fd = INVALID_SOCKET;

    ASSERT_EQ(completedFuture.wait_for(10s), std::future_status::ready);
    const auto result = completedFuture.get();
    ASSERT_EQ(result.readable, 1);
    ASSERT_EQ(result.received, 0);
    ASSERT_EQ(result.sendError, WSAECONNABORTED);
    ASSERT_TRUE(result.connectedBeforeSend);
    EXPECT_TRUE(result.errors.empty()) << result.errors;
#ifndef NDEBUG
    EXPECT_NE(result.trace.find("return on connection aborted, errno=" +
                                std::to_string(WSAECONNABORTED)),
              std::string::npos)
        << result.trace;
#endif
}
#endif

TEST_F(TcpConnectionTest, ForceCloseInStreamCallbackDoesNotSendReturnedData)
{
    struct CallbackResult
    {
        std::size_t returnedBytes;
        bool disconnected;
    };

    auto accepted = std::make_shared<std::promise<TcpConnectionPtr>>();
    auto acceptedFuture = accepted->get_future();
    server_ = std::make_shared<TcpServer>(
        loop_, InetAddress("127.0.0.1", 0), "stream-close-server", true, false);
    server_->setConnectionCallback(
        [accepted](const TcpConnectionPtr &connection) {
            if (connection->connected())
                accepted->set_value(connection);
        });
    server_->setRecvMessageCallback(
        [](const TcpConnectionPtr &, MsgBuffer *buffer) {
            buffer->retrieveAll();
        });
    server_->start();

    auto closed = std::make_shared<std::promise<std::size_t>>();
    auto closedFuture = closed->get_future();
    client_ = std::make_shared<TcpClient>(loop_,
                                          server_->address(),
                                          "stream-close-client");
    client_->setConnectionCallback(
        [closed](const TcpConnectionPtr &connection) {
            if (connection->disconnected())
                closed->set_value(connection->bytesReceived());
        });
    client_->setMessageCallback(
        [](const TcpConnectionPtr &, MsgBuffer *buffer) {
            buffer->retrieveAll();
        });
    client_->connect();
    ASSERT_EQ(acceptedFuture.wait_for(5s), std::future_status::ready);

    auto produced = std::make_shared<std::promise<CallbackResult>>();
    auto producedFuture = produced->get_future();
    loop_->queueInLoop([connection = acceptedFuture.get(), produced]() mutable {
        connection->sendStream(
            [connection, produced, sent = false](
                char *buffer, std::size_t capacity) mutable -> std::size_t {
                if (buffer == nullptr || sent)
                    return 0;
                sent = true;
                const std::size_t length = capacity > 0 ? 1 : 0;
                if (length != 0)
                    buffer[0] = 'x';
                connection->forceClose();
                produced->set_value(
                    CallbackResult{length, connection->disconnected()});
                return length;
            });
        connection.reset();
    });

    ASSERT_EQ(producedFuture.wait_for(5s), std::future_status::ready);
    const auto result = producedFuture.get();
    ASSERT_EQ(result.returnedBytes, 1u);
    ASSERT_TRUE(result.disconnected);
    ASSERT_EQ(closedFuture.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(closedFuture.get(), 0u);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
