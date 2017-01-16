#include <trantor/utils/Logger.h>
#include <stdio.h>
namespace trantor
{
    void defaultOutputFunction(const char *msg,uint64_t length)
    {
        fwrite(msg,1,length,stdout);
    }

    uint64_t Logger::lastSecond_=0;
    std::string Logger::lastTimeString_;
    Logger::LogLevel Logger::logLevel_=DEBUG;
    std::function<void (const char *,uint64_t)> Logger::outputFunc_=defaultOutputFunction;

    void Logger::formatTime() {
        uint64_t now=date_.microSecondsSinceEpoch();
        uint64_t microSec=now%1000000;
        now=now/1000000;
        if(now!=lastSecond_)
        {
            lastSecond_=now;
            lastTimeString_=date_.toFormattedString(false);
        }
        logStream_<<lastTimeString_;
        char tmp[32];
        sprintf(tmp,".%06ld GMT ",microSec);
        logStream_<<tmp;
    }
    static const char* logLevelStr[Logger::LogLevel::NUM_LOG_LEVELS]={
            "TRACE ",
            "DEBUG ",
            "INFO  ",
            "WARN  ",
            "ERROR ",
            "FATAL ",
    };
    Logger::Logger(SourceFile file,int line)
    :sourceFile_(file),
     fileLine_(line),
     level_(INFO)
    {
        formatTime();
        logStream_<<logLevelStr[level_];
    }
    Logger::Logger(SourceFile file, int line, LogLevel level)
            :sourceFile_(file),
             fileLine_(line),
             level_(level)
    {
        formatTime();
        logStream_<<logLevelStr[level_];
    }
    Logger::Logger(SourceFile file, int line, LogLevel level,const char*func)
            :sourceFile_(file),
             fileLine_(line),
             level_(level)
    {
        formatTime();
        logStream_<<logLevelStr[level_]<<"["<<func<<"] ";
    }
    Logger::Logger(SourceFile file, int line, bool isSysErr)
            :sourceFile_(file),
             fileLine_(line),
             level_(FATAL)
    {
        formatTime();
        if (errno != 0)
        {
            logStream_ << strerror(errno) << " (errno=" << errno << ") ";
        }
    }
    Logger::~Logger() {
        logStream_<<" - "<<sourceFile_.data_<<":"<<fileLine_<<std::endl;
        Logger::outputFunc_(logStream_.str().c_str(),logStream_.str().length());
    }
}