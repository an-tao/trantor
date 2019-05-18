#include <trantor/utils/Logger.h>
#include <trantor/utils/AsyncFileLogger.h>
#include <unistd.h>
#include <stdlib.h>
int main()
{
    trantor::AsyncFileLogger asyncFileLogger;
    asyncFileLogger.setFileName("async_test");
    asyncFileLogger.startLogging();
    trantor::Logger::
        setOutputFunction([&](const char *msg,
                              const uint64_t
                                  len) { asyncFileLogger.output(msg, len); },
                          [&]() { asyncFileLogger.flush(); });
    asyncFileLogger.setFileSizeLimit(100000000);
    int i = 0;
    while (i < 1000000)
    {
        i++;
        if (i % 100 == 0)
        {
            LOG_ERROR << "this is the " << i << "th log";
            continue;
        }
        LOG_INFO << "this is the " << i << "th log";
        i++;
        LOG_DEBUG << "this is the " << i << "th log";
        sleep(1);
    }
}
