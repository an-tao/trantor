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
    : logBufferPtr_(new std::string),
      nextBufferPtr_(new std::string)
{
    logBufferPtr_->reserve(MEM_BUFFER_SIZE);
    nextBufferPtr_->reserve(MEM_BUFFER_SIZE);
}
AsyncFileLogger::~AsyncFileLogger()
{
    //std::cout << "~AsyncFileLogger" << std::endl;
    stopFlag_ = true;
    if (threadPtr_)
    {
        cond_.notify_all();
        threadPtr_->join();
    }
    //std::cout << "thread exit" << std::endl;
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
    if (writeBuffers_.size() > 25) //100M bytes logs in buffer
    {
        lostCounter_++;
        return;
    }

    if (lostCounter_ > 0)
    {
        char logErr[128];
#ifdef __linux__
        sprintf(logErr, "%ld log information is lost\n", lostCounter_);
#else
        sprintf(logErr, "%lld log information is lost\n", lostCounter_);
#endif
        lostCounter_ = 0;
        logBufferPtr_->append(logErr);
    }
    logBufferPtr_->append(msg, len);
}
void AsyncFileLogger::flush()
{
    std::lock_guard<std::mutex> guard_(mutex_);
    if (logBufferPtr_->length() > 0)
    {
        //std::cout<<"flush log buffer len:"<<logBufferPtr_->length()<<std::endl;
        swapBuffer();
        cond_.notify_one();
    }
}
void AsyncFileLogger::writeLogToFile(const StringPtr buf)
{
    if (!loggerFilePtr_)
    {
        loggerFilePtr_ = std::unique_ptr<LoggerFile>(new LoggerFile(filePath_, fileBaseName_, fileExtName_));
    }
    loggerFilePtr_->writeLog(buf);
    if (loggerFilePtr_->getLength() > sizeLimit_)
    {
        loggerFilePtr_.reset();
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
                if (cond_.wait_for(lock, std::chrono::seconds(LOG_FLUSH_TIMEOUT)) == std::cv_status::timeout)
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
        if (loggerFilePtr_)
            loggerFilePtr_->flush();
    }
}
void AsyncFileLogger::startLogging()
{
    threadPtr_ = std::unique_ptr<std::thread>(
        new std::thread(std::bind(&AsyncFileLogger::logThreadFunc, this)));
}

AsyncFileLogger::LoggerFile::LoggerFile(const std::string &filePath, const std::string &fileBaseName, const std::string &fileExtName)
    : createDate_(Date::date()),
      filePath_(filePath),
      fileBaseName_(fileBaseName),
      fileExtName_(fileExtName)
{
    fileFullName_ = filePath + fileBaseName + fileExtName;
    fp_ = fopen(fileFullName_.c_str(), "a");
    if (fp_ != NULL)
    {
    }
    else
    {
        std::cout << strerror(errno) << std::endl;
    }
}
uint64_t AsyncFileLogger::LoggerFile::fileSeq_ = 0;
void AsyncFileLogger::LoggerFile::writeLog(const StringPtr buf)
{
    if (fp_)
    {
        //std::cout<<"write "<<buf->length()<<" bytes to file"<<std::endl;
        fwrite(buf->c_str(), 1, buf->length(), fp_);
    }
}
void AsyncFileLogger::LoggerFile::flush()
{
    if (fp_)
    {
        fflush(fp_);
    }
}
uint64_t AsyncFileLogger::LoggerFile::getLength()
{
    if (fp_)
        return ftell(fp_);
    return 0;
}
AsyncFileLogger::LoggerFile::~LoggerFile()
{
    if (fp_)
    {
        fclose(fp_);
        char seq[10];
#ifdef __linux__
        sprintf(seq, ".%06ld", fileSeq_ % 1000000);
#else
        sprintf(seq, ".%06lld", fileSeq_ % 1000000);
#endif
        fileSeq_++;
        std::string newName = filePath_ + fileBaseName_ + "." + createDate_.toCustomedFormattedString("%y%m%d-%H%M%S") + std::string(seq) + fileExtName_;
        rename(fileFullName_.c_str(), newName.c_str());
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
