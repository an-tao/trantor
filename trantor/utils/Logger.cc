#include <trantor/utils/Logger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

namespace trantor
{
    void defaultOutputFunction(const std::stringstream &out)
    {
        fwrite(out.str().c_str(),1,out.str().length(),stdout);
    }
    void defaultFlushFunction()
    {
        fflush(stdout);
    }
    static __thread time_t lastSecond_=0;
    static __thread char lastTimeString_[32]={0};
    static __thread pid_t threadId_ = 0;
    Logger::LogLevel Logger::logLevel_=DEBUG;
    std::function<void (const std::stringstream &)> Logger::outputFunc_=defaultOutputFunction;
    std::function<void ()> Logger::flushFunc_=defaultFlushFunction;
    void Logger::formatTime() {
        uint64_t now=date_.microSecondsSinceEpoch();
        uint64_t microSec=now%1000000;
        now=now/1000000;
        if(now!=lastSecond_)
        {
            lastSecond_=now;
            strncpy(lastTimeString_,date_.toFormattedString(false).c_str(),sizeof(lastTimeString_));
        }
        logStream_<<lastTimeString_;
        char tmp[32];
        sprintf(tmp,".%06ld UTC ",microSec);
        logStream_<<tmp;
        if(threadId_==0)
            threadId_=static_cast<pid_t>(::syscall(SYS_gettid));
        logStream_<<threadId_;
    }
    static const char* logLevelStr[Logger::LogLevel::NUM_LOG_LEVELS]={
            " TRACE ",
            " DEBUG ",
            " INFO  ",
            " WARN  ",
            " ERROR ",
            " FATAL ",
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
            logStream_<<logLevelStr[level_];
            logStream_ << strerror(errno) << " (errno=" << errno << ") ";
        }
    }
    Logger::~Logger() {
        logStream_<<" - "<<sourceFile_.data_<<":"<<fileLine_<<std::endl;
        Logger::outputFunc_(logStream_);
        if(level_>=ERROR)
            Logger::flushFunc_();
    }
}