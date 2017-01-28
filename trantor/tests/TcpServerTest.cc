#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoop.h>
#include <string>
#include <iostream>
using namespace trantor;
int main()
{
    Logger::setLogLevel(Logger::TRACE);
    EventLoop loop;
    InetAddress addr(8888);
    TcpServer server(&loop,addr,"test");
    server.setRecvMessageCallback([](const TcpConnectionPtr &connectionPtr,MsgBuffer *buffer){
        //LOG_DEBUG<<"recv callback!";
        std::cout<<std::string(buffer->peek(),buffer->readableBytes());
        connectionPtr->send(buffer->peek(),buffer->readableBytes());
        buffer->retrieveAll();
    });
    server.start();
    loop.loop();
}