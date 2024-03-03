#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoopThread.h>
#include <memory>
#include <string>
#include <iostream>
#include "trantor/net/Pipeline.h"
using namespace trantor;
#define USE_IPV6 0
#define AUTO_UNPACK 1
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
#if AUTO_UNPACK
    trantor::PipelineSetting setting;
    setting.mode_ = trantor::PipelineMode::UnpackByDelimiter;
    setting.delimiter_ = "\n";
    setting.packageMaxLength_ = 2048;
    // -------------------------------------------------
    // setting.mode_ = trantor::PipelineMode::UnpackByFixedLength;
    // setting.fixedLength = 5;
    // -------------------------------------------------
    setting.noticeCallback_ = [](std::string error) {
        std::cout << "auto unpack error: " << error << std::endl;
    };
    server.setPipeline(std::make_unique<trantor::Pipeline>(setting));
#endif
    server.setBeforeListenSockOptCallback([](int fd) {
        std::cout << "setBeforeListenSockOptCallback:" << fd << std::endl;
    });
    server.setAfterAcceptSockOptCallback([](int fd) {
        std::cout << "afterAcceptSockOptCallback:" << fd << std::endl;
    });
    server.setRecvMessageCallback(
        [](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer) {
#if AUTO_UNPACK
            LOG_DEBUG << "recv auto unpack: "
                      << std::string(buffer->peek(), buffer->readableBytes());
            connectionPtr->send(buffer->peek(), buffer->readableBytes());
#else
            // LOG_DEBUG<<"recv callback!";
            std::cout << std::string(buffer->peek(), buffer->readableBytes());
            connectionPtr->send(buffer->peek(), buffer->readableBytes());
            buffer->retrieveAll();
        // connectionsetting.forceClose();
#endif
        });
    server.setConnectionCallback([](const TcpConnectionPtr &connPtr) {
        if (connPtr->connected())
        {
            LOG_DEBUG << "New connection";
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
