#include <trantor/net/EventLoop.h>
#include <trantor/utils/Logger.h>
#include <iostream>
int main()
{
    trantor::EventLoop loop;
    loop.runEvery(3,[](){
        LOG_DEBUG<<" runEvery 3s";
    });
    loop.runAt(trantor::Date::date().after(10),[](){
        LOG_DEBUG<<"runAt 10s later";
    });
    loop.runAfter(5,[](){
        std::cout<<"runAt 5s later"<<std::endl;
    });
    loop.runEvery(1,[](){
        std::cout<<"runEvery 1s"<<std::endl;
    });
    loop.runAfter(4,[](){
        std::cout<<"runAfter 4s"<<std::endl;
    });
    loop.loop();
}