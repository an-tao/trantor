#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoop.h>
using namespace trantor;
int main()
{
    Logger::setLogLevel(Logger::TRACE);
    EventLoop loop;
    InetAddress addr("127.0.0.1",8888);
    TcpServer server(&loop,addr,"test");
    server.start();
    loop.loop();
}