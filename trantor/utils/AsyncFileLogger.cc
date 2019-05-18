/**
 *
 *  AsyncFileLogger.cc
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

#include <trantor/utils/AsyncFileLogger.h>
#ifndef __WINDOWS__
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#endif
#include <string.h>
#include <iostream>
#include <functional>

#define LOG_FLUSH_TIMEOUT 1
#define MEM_BUFFER_SIZE 4 * 1024 * 1024
using namespace trantor;

AsyncFileLogger::AsyncFileLogger()
    : logBufferPtr_(new std::string), nextBufferPtr_(new std::string)
{
    logBufferPtr_->reserve(MEM_BUFFER_SIZE);
    nextBufferPtr_->reserve(MEM_BUFFER_SIZE);
}

AsyncFileLogger::~AsyncFileLogger()
{
    // std::cout << "~AsyncFileLogger" << std::endl;
    stopFlag_ = true;
    if (threadPtr_)
    {
        cond_.notify_all();
        threadPtr_->join();
    }
    // std::cout << "thread exit" << std::endl;
    {
        std::lock_guard<std::mutex> guard_(mutex_);
        if (logBufferPtr_->length() > 0)
        {
            writeBuffers_.push(logBufferPtr_);
        }
        if (writeBuffers_.size() > 0)
        {
            StringPtr tmpPtr = (StringPtr &&) writeBuffers_.front();
            writeBuffers_.pop();
            writeLogToFile(tmpPtr);
        }
    }
}

void AsyncFileLogger::output(const char *msg, const uint64_t len)
{
    std::lock_guard<std::mutex> guard_(mutex_);
    if (len > MEM_BUFFER_SIZE)
        return;
    if (!logBufferPtr_)
    {
        logBufferPtr_ = std::make_shared<std::string>();
        logBufferPtr_->reserve(MEM_BUFFER_SIZE);
    }
    if (logBufferPtr_->capacity() - logBufferPtr_->length() < len)
    {
        swapBuffer();
        cond_.notify_one();
    }
    if (writeBuffers_.size() > 25)  // 100M bytes logs in buffer
    {
        _lostCounter++;
        return;
    }

    if (_lostCounter > 0)
    {
        char logErr[128];
        auto strlen =
            sprintf(logErr,
                    "%llu log information is lost\n",
                    static_cast<long long unsigned int>(_lostCounter));
        _lostCounter = 0;
        logBufferPtr_->append(logErr, strlen);
    }
    logBufferPtr_->append(msg, len);
}

void AsyncFileLogger::flush()
{
    std::lock_guard<std::mutex> guard_(mutex_);
    if (logBufferPtr_->length() > 0)
    {
        // std::cout<<"flush log buffer
        // len:"<<logBufferPtr_->length()<<std::endl;
        swapBuffer();
        cond_.notify_one();
    }
}

void AsyncFileLogger::writeLogToFile(const StringPtr buf)
{
    if (!_loggerFilePtr)
    {
        _loggerFilePtr = std::unique_ptr<LoggerFile>(
            new LoggerFile(_filePath, _fileBaseName, _fileExtName));
    }
    _loggerFilePtr->writeLog(buf);
    if (_loggerFilePtr->getLength() > sizeLimit_)
    {
        _loggerFilePtr.reset();
    }
}

void AsyncFileLogger::logThreadFunc()
{
#ifdef __linux__
    prctl(PR_SET_NAME, "AsyncFileLogger");
#endif
    while (!stopFlag_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (writeBuffers_.size() == 0 && !stopFlag_)
            {
                if (cond_.wait_for(lock,
                                   std::chrono::seconds(LOG_FLUSH_TIMEOUT)) ==
                    std::cv_status::timeout)
                {
                    if (logBufferPtr_->length() > 0)
                    {
                        swapBuffer();
                    }
                    break;
                }
            }
            tmpBuffers_.swap(writeBuffers_);
        }

        while (tmpBuffers_.size() > 0)
        {
            StringPtr tmpPtr = (StringPtr &&) tmpBuffers_.front();
            tmpBuffers_.pop();
            writeLogToFile(tmpPtr);
            tmpPtr->clear();
            {
                std::unique_lock<std::mutex> lock(mutex_);
                nextBufferPtr_ = tmpPtr;
            }
        }
        if (_loggerFilePtr)
            _loggerFilePtr->flush();
    }
}

void AsyncFileLogger::startLogging()
{
    threadPtr_ = std::unique_ptr<std::thread>(
        new std::thread(std::bind(&AsyncFileLogger::logThreadFunc, this)));
}

AsyncFileLogger::LoggerFile::LoggerFile(const std::string &filePath,
                                        const std::string &fileBaseName,
                                        const std::string &fileExtName)
    : _creationDate(Date::date()),
      _filePath(filePath),
      _fileBaseName(fileBaseName),
      _fileExtName(fileExtName)
{
    _fileFullName = filePath + fileBaseName + fileExtName;
    _fp = fopen(_fileFullName.c_str(), "a");
    if (_fp == NULL)
    {
        std::cout << strerror(errno) << std::endl;
    }
}

uint64_t AsyncFileLogger::LoggerFile::_fileSeq = 0;
void AsyncFileLogger::LoggerFile::writeLog(const StringPtr buf)
{
    if (_fp)
    {
        // std::cout<<"write "<<buf->length()<<" bytes to file"<<std::endl;
        fwrite(buf->c_str(), 1, buf->length(), _fp);
    }
}

void AsyncFileLogger::LoggerFile::flush()
{
    if (_fp)
    {
        fflush(_fp);
    }
}

uint64_t AsyncFileLogger::LoggerFile::getLength()
{
    if (_fp)
        return ftell(_fp);
    return 0;
}

AsyncFileLogger::LoggerFile::~LoggerFile()
{
    if (_fp)
    {
        fclose(_fp);
        char seq[10];
        sprintf(seq,
                ".%06llu",
                static_cast<long long unsigned int>(_fileSeq % 1000000));
        _fileSeq++;
        std::string newName =
            _filePath + _fileBaseName + "." +
            _creationDate.toCustomedFormattedString("%y%m%d-%H%M%S") +
            std::string(seq) + _fileExtName;
        rename(_fileFullName.c_str(), newName.c_str());
    }
}

void AsyncFileLogger::swapBuffer()
{
    writeBuffers_.push(logBufferPtr_);
    if (nextBufferPtr_)
    {
        logBufferPtr_ = nextBufferPtr_;
        nextBufferPtr_.reset();
        logBufferPtr_->clear();
    }
    else
    {
        logBufferPtr_ = std::make_shared<std::string>();
        logBufferPtr_->reserve(MEM_BUFFER_SIZE);
    }
}
