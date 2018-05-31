#include <trantor/utils/Logger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

namespace trantor
{
    // helper class for known string length at compile time
    class T
    {
    public:
        T(const char* str, unsigned len)
                :str_(str),
                 len_(len)
        {
            assert(strlen(str) == len_);
        }

        const char* str_;
        const unsigned len_;
    };

    __thread char t_errnobuf[512];

    const char* strerror_tl(int savedErrno)
    {
        return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
    }

inline LogStream& operator<<(LogStream& s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
    s.append(v.data_, v.size_);
    return s;
}
}
using namespace trantor;
void defaultOutputFunction(const char *msg,const uint64_t len)
{
    fwrite(msg,1,len,stdout);
}
void defaultFlushFunction()
{
    fflush(stdout);
}
static __thread uint64_t lastSecond_=0;
static __thread char lastTimeString_[32]={0};
static __thread pid_t threadId_ = 0;
//   static __thread LogStream logStream_;
#ifdef RELEASE
Logger::LogLevel Logger::logLevel_=LogLevel::INFO;
#else
Logger::LogLevel Logger::logLevel_=LogLevel::DEBUG;
#endif
std::function<void (const char *msg,const uint64_t len)> Logger::outputFunc_=defaultOutputFunction;
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
    logStream_<<T(lastTimeString_,17);
    char tmp[32];
    sprintf(tmp,".%06ld UTC ",microSec);
    logStream_<<T(tmp,12);
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
    logStream_<<T(logLevelStr[level_],7);
}
Logger::Logger(SourceFile file, int line, LogLevel level)
        :sourceFile_(file),
         fileLine_(line),
         level_(level)
{
    formatTime();
    logStream_<<T(logLevelStr[level_],7);
}
Logger::Logger(SourceFile file, int line, LogLevel level,const char*func)
        :sourceFile_(file),
         fileLine_(line),
         level_(level)
{
    formatTime();
    logStream_<<T(logLevelStr[level_],7)<<"["<<func<<"] ";
}
Logger::Logger(SourceFile file, int line, bool isSysErr)
        :sourceFile_(file),
         fileLine_(line),
         level_(FATAL)
{
    formatTime();
    if (errno != 0)
    {
        logStream_<<T(logLevelStr[level_],7);
        logStream_ << strerror(errno) << " (errno=" << errno << ") ";
    }
}
Logger::~Logger() {
    logStream_<<T(" - ",3)<<sourceFile_<<':'<<fileLine_<<'\n';
    if(Logger::outputFunc_)
        Logger::outputFunc_(logStream_.buffer().data(),logStream_.buffer().length());
    else
        defaultOutputFunction(logStream_.buffer().data(),logStream_.buffer().length());
    if(level_>=ERROR)
        Logger::flushFunc_();
    //logStream_.resetBuffer();
}
LogStream & Logger::stream() {
    return logStream_;
}
