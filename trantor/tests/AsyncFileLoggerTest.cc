#include <trantor/utils/Logger.h>
#include <trantor/utils/AsyncFileLogger.h>
#include <stdlib.h>
#include <thread>
int main()
{
    trantor::AsyncFileLogger asyncFileLogger;
    asyncFileLogger.setFileName("async_test");
    asyncFileLogger.startLogging();
    trantor::Logger::setOutputFunction([&](const char *msg,const uint64_t len){
        asyncFileLogger.output(msg,len);
    },[](){});
//    LOG_DEBUG<<"debug log!"<<1;
//    LOG_TRACE<<"trace log!"<<2;
//    LOG_INFO<<"info log!"<<3;
//    LOG_WARN<<"warning log!"<<4;
//    if(1)
//        LOG_ERROR<<"error log!"<<5;
//    std::thread thread_([](){
//        LOG_FATAL<<"fatal log!"<<6;
//    });
//
//    FILE *fp=fopen("/notexistfile","rb");
//    if(fp==NULL)
//    {
//        LOG_SYSERR<<"syserr log!"<<7;
//    }
    std::thread thread1_([](){
        int i=0;
        while(i<1000000)
        {
            i++;
            LOG_INFO<<"this is "<<i<<"th log";
        }
    });
 //   thread_.join();
    thread1_.join();
}