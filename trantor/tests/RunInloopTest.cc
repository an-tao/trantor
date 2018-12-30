//
// Created by antao on 1/14/17.
//

#include <trantor/net/EventLoop.h>
#include <unistd.h>

#include <iostream>
#include <thread>
int main()
{
    trantor::EventLoop loop;
    std::thread thread([&loop]() {
        sleep(3);
        loop.runInLoop([&loop]() {
            std::cout << "runInLoop called in other thread" << std::endl;
            loop.queueInLoop([]() {
                std::cout << "queueInLoop in runInLoop" << std::endl;
            });
        });
    });
    loop.runInLoop([]() {
        std::cout << "runInLoop 1" << std::endl;
    });
    loop.runInLoop([]() {
        std::cout << "runInLoop 2" << std::endl;
    });
    loop.queueInLoop([]() {
        std::cout << "queueInLoop 1" << std::endl;
    });
    loop.runAfter(1.5, []() {
        std::cout << "run after 1.5" << std::endl;
    });
    loop.loop();
}
