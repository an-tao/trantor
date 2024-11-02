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
    server.setBeforeListenSockOptCallback([](int fd) {
        std::cout << "setBeforeListenSockOptCallback:" << fd << std::endl;
    });
    server.setAfterAcceptSockOptCallback([](int fd) {
        std::cout << "afterAcceptSockOptCallback:" << fd << std::endl;
    });
    server.setRecvMessageCallback(
        [&server](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
            std::string input;
            std::string user;
            LOG_DEBUG<<"recv callback!";
            input = std::string(buffer->peek(), buffer->readableBytes());
            user = server.ParseInput(connectionPtr, input);
            std::cout << user << ": " << input << std::endl;
            connectionPtr->send(buffer->peek(), buffer->readableBytes());
            buffer->retrieveAll();
            // connectionPtr->forceClose();
        });
    server.setConnectionCallback([&server](const TcpConnectionPtr &connPtr) {
        if (connPtr->connected())
        {
            LOG_DEBUG << "New connection";
            server.AddUser(connPtr);
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
