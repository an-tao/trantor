/**
 *
 *  AsyncFileLogger.h
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

#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/Date.h>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <sstream>
#include <memory>
#include <queue>

namespace trantor
{

typedef std::shared_ptr<std::string> StringPtr;
typedef std::queue<StringPtr> StringPtrQueue;
class AsyncFileLogger : NonCopyable
{
  public:
    //static AsyncFileLogger* getInstance();
    void output(const char *msg, const uint64_t len);
    void flush();
    void startLogging();
    void setFileSizeLimit(uint64_t limit) { sizeLimit_ = limit; }
    void setFileName(const std::string &baseName, const std::string &extName = ".log", const std::string &path = "./")
    {
        _fileBaseName = baseName;
        extName[0] == '.' ? _fileExtName = extName : _fileExtName = std::string(".") + extName;
        _filePath = path;
        if (_filePath.length() == 0)
            _filePath = "./";
        if (_filePath[_filePath.length() - 1] != '/')
            _filePath = _filePath + "/";
    }
    ~AsyncFileLogger();
    AsyncFileLogger();

  protected:
    std::mutex mutex_;
    std::condition_variable cond_;
    StringPtr logBufferPtr_;
    StringPtr nextBufferPtr_;
    StringPtrQueue writeBuffers_;
    StringPtrQueue tmpBuffers_;
    void writeLogToFile(const StringPtr buf);
    std::unique_ptr<std::thread> threadPtr_;
    bool stopFlag_ = false;
    void logThreadFunc();
    std::string _filePath = "./";
    std::string _fileBaseName = "trantor";
    std::string _fileExtName = ".log";
    uint64_t sizeLimit_ = 20 * 1024 * 1024;
    class LoggerFile : NonCopyable
    {
      public:
        LoggerFile(const std::string &filePath, const std::string &fileBaseName, const std::string &fileExtName);
        ~LoggerFile();
        void writeLog(const StringPtr buf);
        uint64_t getLength();
        explicit operator bool() const { return _fp != NULL; }
        void flush();

      protected:
        FILE *_fp = NULL;
        Date _creationDate;
        std::string _fileFullName;
        std::string _filePath;
        std::string _fileBaseName;
        std::string _fileExtName;
        static uint64_t _fileSeq;
    };
    std::unique_ptr<LoggerFile> _loggerFilePtr;

    uint64_t _lostCounter = 0;
    void swapBuffer();
};

} // namespace trantor
