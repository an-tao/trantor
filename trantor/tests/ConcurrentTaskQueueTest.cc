#include <trantor/utils/ConcurrentTaskQueue.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <trantor/utils/Logger.h>
#include <unistd.h>
#include <atomic>
int main()
{
    trantor::ConcurrentTaskQueue queue(5, "concurrT");
    std::atomic_int sum;
    sum = 0;
    for (int i = 0; i < 4; i++)
    {
        queue.runTaskInQueue([&sum]() {
            LOG_DEBUG << "add sum";
            for (int i = 0; i < 10000; i++)
            {
                sum++;
            }
        });
    }

    queue.runTaskInQueue([&sum]() {
        for (int i = 0; i < 20; i++)
        {
            LOG_DEBUG << "sum=" << sum;
            usleep(10);
        }
    });

    getc(stdin);
}
