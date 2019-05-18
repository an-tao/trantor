/**
 *
 *  Logger.cc
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#include <trantor/utils/Logger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>
#ifdef __FreeBSD__
#include <pthread_np.h>
#endif

namespace trantor
{
// helper class for known string length at compile time
class T
{
  public:
    T(const char *str, unsigned len) : str_(str), len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char *str_;
    const unsigned len_;
};

const char *strerror_tl(int savedErrno)
{
    return strerror(savedErrno);
}

inline LogStream &operator<<(LogStream &s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
{
    s.append(v.data_, v.size_);
    return s;
}
}  // namespace trantor
using namespace trantor;

static __thread uint64_t lastSecond_ = 0;
static __thread char lastTimeString_[32] = {0};
#ifdef __linux__
static __thread pid_t threadId_ = 0;
#else
static __thread uint64_t threadId_ = 0;
#endif
//   static __thread LogStream logStream_;

void Logger::formatTime()
{
    uint64_t now = date_.microSecondsSinceEpoch();
    uint64_t microSec = now % 1000000;
    now = now / 1000000;
    if (now != lastSecond_)
    {
        lastSecond_ = now;
        strncpy(lastTimeString_,
                date_.toFormattedString(false).c_str(),
                sizeof(lastTimeString_) - 1);
    }
    logStream_ << T(lastTimeString_, 17);
    char tmp[32];
    sprintf(tmp, ".%06llu UTC ", static_cast<long long unsigned int>(microSec));
    logStream_ << T(tmp, 12);
#ifdef __linux__
    if (threadId_ == 0)
        threadId_ = static_cast<pid_t>(::syscall(SYS_gettid));
#elif defined __FreeBSD__
    if (threadId_ == 0)
    {
        threadId_ = pthread_getthreadid_np();
    }
#else
    if (threadId_ == 0)
    {
        pthread_threadid_np(NULL, &threadId_);
    }
#endif
    logStream_ << threadId_;
}
static const char *logLevelStr[Logger::LogLevel::NUM_LOG_LEVELS] = {
    " TRACE ",
    " DEBUG ",
    " INFO  ",
    " WARN  ",
    " ERROR ",
    " FATAL ",
};
Logger::Logger(SourceFile file, int line)
    : sourceFile_(file), fileLine_(line), level_(INFO)
{
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(SourceFile file, int line, LogLevel level)
    : sourceFile_(file), fileLine_(line), level_(level)
{
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
    : sourceFile_(file), fileLine_(line), level_(level)
{
    formatTime();
    logStream_ << T(logLevelStr[level_], 7) << "[" << func << "] ";
}
Logger::Logger(SourceFile file, int line, bool isSysErr)
    : sourceFile_(file), fileLine_(line), level_(FATAL)
{
    formatTime();
    if (errno != 0)
    {
        logStream_ << T(logLevelStr[level_], 7);
        logStream_ << strerror(errno) << " (errno=" << errno << ") ";
    }
}
Logger::~Logger()
{
    logStream_ << T(" - ", 3) << sourceFile_ << ':' << fileLine_ << '\n';
    Logger::_outputFunc()(logStream_.bufferData(), logStream_.bufferLength());
    if (level_ >= ERROR)
        Logger::_flushFunc()();
    // logStream_.resetBuffer();
}
LogStream &Logger::stream()
{
    return logStream_;
}
