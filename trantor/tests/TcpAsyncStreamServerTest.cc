#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoopThread.h>
#include <string>
#include <iostream>
using namespace trantor;
#define USE_IPV6 0
int main()
{
    LOG_DEBUG << "test start";
    Logger::setLogLevel(Logger::kTrace);
    EventLoopThread loopThread;
    loopThread.run();
#if USE_IPV6
    InetAddress addr(8888, true, true);
#else
    InetAddress addr(8888);
#endif
    TcpServer server(loopThread.getLoop(), addr, "test");

    server.setRecvMessageCallback(
        [](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
            // LOG_DEBUG<<"recv callback!";
            std::cout << std::string(buffer->peek(), buffer->readableBytes());
            connectionPtr->send(buffer->peek(), buffer->readableBytes());
            buffer->retrieveAll();
            // connectionPtr->forceClose();
        });
    server.setConnectionCallback([](const TcpConnectionPtr &connPtr) {
        if (connPtr->connected())
        {
            LOG_DEBUG << "New connection";
            auto stream = connPtr->sendAsyncStream();
            stream->send("hello world 1...");
            std::thread([stream = std::move(stream)] {
                for (int i = 2; i < 10; i++)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    stream->send("hello world " + std::to_string(i) + "...");
                }
                stream->close();
            }).detach();
            connPtr->send("hello world");
        }
        else if (connPtr->disconnected())
        {
            LOG_DEBUG << "connection disconnected";
        }
    });
    server.setIoLoopNum(3);
    server.start();
    loopThread.wait();
}
