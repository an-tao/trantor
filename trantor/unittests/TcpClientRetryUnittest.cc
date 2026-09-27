#include <trantor/net/EventLoopThread.h>
#include <trantor/net/TcpClient.h>
#include <trantor/net/TcpServer.h>
// Connector.h is not an installed header; include it by path. TcpClient
// exposes no way to invoke restart(), so the regression test below drives
// the connector directly.
#include "../net/inner/Connector.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#endif

using namespace trantor;
using namespace std::chrono_literals;

namespace
{
template <typename Pred>
bool waitFor(Pred pred, std::chrono::milliseconds timeout)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!pred())
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(10ms);
    }
    return true;
}

void closeSocket(int sockfd)
{
#ifndef _WIN32
    ::close(sockfd);
#else
    closesocket(sockfd);
#endif
}

class TcpClientRetry : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        loopThread_.run();
        loop_ = loopThread_.getLoop();
        // The acceptor binds in the constructor but only listens in start(),
        // so until startServer() is called the port is reserved and any
        // connection attempt to it is refused.
        server_ = std::make_shared<TcpServer>(loop_,
                                              InetAddress("127.0.0.1", 0),
                                              "retry-test-server");
        server_->setConnectionCallback([this](const TcpConnectionPtr &conn) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (conn->connected())
            {
                serverConn_ = conn;
                ++serverAccepts_;
            }
            else if (serverConn_ == conn)
            {
                // The socket is only closed once the last reference to the
                // connection is gone.
                serverConn_.reset();
            }
        });
        server_->setRecvMessageCallback(
            [](const TcpConnectionPtr &, MsgBuffer *buf) {
                buf->retrieveAll();
            });
        port_ = server_->address().toPort();

        client_ = std::make_shared<TcpClient>(loop_,
                                              InetAddress("127.0.0.1", port_),
                                              "retry-test-client");
        client_->enableRetry();
        client_->setConnectionCallback([this](const TcpConnectionPtr &conn) {
            if (conn->connected())
                ++clientConnects_;
        });
        client_->setConnectionErrorCallback([this]() { ++clientErrors_; });
        client_->setMessageCallback([](const TcpConnectionPtr &,
                                       MsgBuffer *buf) { buf->retrieveAll(); });
    }

    void TearDown() override
    {
        client_->stop();
        server_->stop();
        std::promise<void> done;
        loop_->runInLoop([this, &done]() {
            client_.reset();
            done.set_value();
        });
        done.get_future().wait();
        std::this_thread::sleep_for(50ms);
    }

    void startServer()
    {
        server_->start();
    }

    TcpConnectionPtr serverConn()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return serverConn_;
    }

    EventLoopThread loopThread_;
    EventLoop *loop_{nullptr};
    uint16_t port_{0};
    std::shared_ptr<TcpServer> server_;
    std::shared_ptr<TcpClient> client_;
    std::mutex mutex_;
    TcpConnectionPtr serverConn_;
    std::atomic<int> serverAccepts_{0};
    std::atomic<int> clientConnects_{0};
    std::atomic<int> clientErrors_{0};
};
}  // namespace

TEST_F(TcpClientRetry, RetriesFailedInitialConnection)
{
    client_->connect();
    ASSERT_TRUE(waitFor([this]() { return clientErrors_ >= 1; }, 3s));
    startServer();
    EXPECT_TRUE(waitFor([this]() { return clientConnects_ >= 1; }, 5s));
}

TEST_F(TcpClientRetry, ReconnectsAfterConnectionIsLost)
{
    startServer();
    client_->connect();
    ASSERT_TRUE(waitFor([this]() { return serverAccepts_ == 1; }, 3s));
    serverConn()->forceClose();
    EXPECT_TRUE(waitFor([this]() { return serverAccepts_ >= 2; }, 5s));
}

TEST_F(TcpClientRetry, DisconnectDoesNotReconnect)
{
    startServer();
    client_->connect();
    ASSERT_TRUE(waitFor([this]() { return clientConnects_ == 1; }, 3s));
    client_->disconnect();
    // Longer than the initial retry delay (500 ms).
    std::this_thread::sleep_for(1500ms);
    EXPECT_EQ(serverAccepts_, 1);
}

TEST_F(TcpClientRetry, StopCancelsPendingRetry)
{
    client_->connect();
    // After two failures a retry is scheduled 1000 ms ahead.
    ASSERT_TRUE(waitFor([this]() { return clientErrors_ >= 2; }, 3s));
    client_->stop();
    startServer();
    std::this_thread::sleep_for(2500ms);
    EXPECT_EQ(serverAccepts_, 0);
    EXPECT_EQ(clientConnects_, 0);
}

TEST_F(TcpClientRetry, RestartAfterStopDoesNotReconnect)
{
    startServer();

    auto connections = std::make_shared<std::atomic<int>>(0);
    auto connector =
        std::make_shared<Connector>(loop_,
                                    InetAddress("127.0.0.1", port_),
                                    true);
    connector->setNewConnectionCallback([connections](int sockfd) {
        ++(*connections);
        closeSocket(sockfd);
    });

    connector->start();
    ASSERT_TRUE(waitFor([&]() { return *connections == 1; }, 3s));

    // stop() clears the connection intent before restart() is queued, so a
    // restart must not connect again.
    connector->stop();
    loop_->queueInLoop([connector]() { connector->restart(); });

    std::this_thread::sleep_for(1s);
    EXPECT_EQ(*connections, 1);
    EXPECT_EQ(serverAccepts_, 1);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
