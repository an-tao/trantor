#include <trantor/net/TcpServer.h>
#include <trantor/utils/Logger.h>
#include <trantor/net/EventLoopThread.h>
#include <string>
#include <iostream>
#include <thread>
#include <sys/stat.h>
#include <fcntl.h>

using namespace trantor;
#define USE_IPV6 0
int main(int argc, char* argv[])
{
    if(argc<2)
    {
        std::cout<<"usage:"<<argv[0]<<" filename"<<std::endl;
        return 1;
    }
    std::cout<<"filename:"<<argv[1]<<std::endl;
    struct stat filestat;
    if(stat(argv[1], &filestat)<0)
    {
        perror("");
        exit(1);
    }
    std::cout<<"file len="<<filestat.st_size<<std::endl;
    int fd=open(argv[1],O_RDONLY);
    if(fd<0)
    {
        perror("");
        exit(1);
    }
    LOG_DEBUG<<"test start";

    Logger::setLogLevel(Logger::TRACE);
    EventLoopThread loopThread;
    loopThread.run();

#if USE_IPV6
    InetAddress addr(8888,true,true);
#else
    InetAddress addr(8888);
#endif
    TcpServer server(loopThread.getLoop(),addr,"test");
    server.setRecvMessageCallback([](const TcpConnectionPtr &connectionPtr,MsgBuffer *buffer){
        //LOG_DEBUG<<"recv callback!";
    });
    server.setConnectionCallback([=](const TcpConnectionPtr& connPtr){
        if(connPtr->connected())
        {
            LOG_DEBUG<<"New connection";
            std::thread t([=](){
                for(int i=0;i<1;i++)
                {
                    connPtr->sendFile(fd,0,filestat.st_size);
                    char str[64];
                    sprintf(str,"%d files sent!\n",i+1);
                    connPtr->send(str,strlen(str));
                }
            });
            t.detach();
        }
        else if(connPtr->disconnected())
        {
            LOG_DEBUG<<"connection disconnected";
        }
    });
    server.setIoLoopNum(3);
    server.start();
    loopThread.wait();
}