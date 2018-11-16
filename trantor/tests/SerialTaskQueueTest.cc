#include <trantor/utils/SerialTaskQueue.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <trantor/utils/Logger.h>
int main()
{
    trantor::Logger::setLogLevel(trantor::Logger::TRACE);
    trantor::SerialTaskQueue queue1("test queue1");
    trantor::SerialTaskQueue queue2("");
    queue1.runTaskInQueue([&]() {
        for (int i = 0; i < 5; i++)
        {
            sleep(1);
            printf("task(%s) i=%d\n", queue1.getName().c_str(), i);
        }
    });
    queue2.runTaskInQueue([&]() {
        for (int i = 0; i < 5; i++)
        {
            sleep(1);
            printf("task(%s) i=%d\n", queue2.getName().c_str(), i);
        }
    });
    queue1.waitAllTasksFinished();
    queue2.waitAllTasksFinished();
}
