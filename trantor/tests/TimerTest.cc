#include <trantor/net/EventLoop.h>
#include <iostream>
int main()
{
    trantor::EventLoop loop;
    loop.runEvery(3,[](){
        std::cout<<"runEvery 3s"<<std::endl;
    });
    loop.runAt(trantor::Date::date().after(10),[](){
        std::cout<<"runAt 10s later"<<std::endl;
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