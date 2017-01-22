#include <trantor/utils/AsyncFileLogger.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/prctl.h>
#define LOG_FLUSH_TIMEOUT 1
#define MEM_BUFFER_SIZE 3*1024*1024
namespace trantor {
    AsyncFileLogger::AsyncFileLogger()
    :logBufferPtr_(new std::string)
    {
        logBufferPtr_->reserve(MEM_BUFFER_SIZE);
    }
    AsyncFileLogger::~AsyncFileLogger()
    {
        stopFlag_=true;
        if(threadPtr_)
        {
            stopFlag_=true;
            cond_.notify_one();
            threadPtr_->join();
        }
        std::lock_guard<std::mutex> guard_(mutex_);
        if(logBufferPtr_->length()>0)
        {
            writeBuffers_.push(logBufferPtr_);
        }
        if(writeBuffers_.size()>0)
        {
            StringPtr tmpPtr=(StringPtr &&)writeBuffers_.front();
            writeBuffers_.pop();
            writeLogToFile(tmpPtr);
        }
    }
    void AsyncFileLogger::output(const char *msg,const uint64_t len) {
        std::lock_guard<std::mutex> guard_(mutex_);
        if(len>MEM_BUFFER_SIZE)
            return;
        if(!logBufferPtr_)
        {
            logBufferPtr_=std::make_shared<std::string>();
            logBufferPtr_->reserve(MEM_BUFFER_SIZE);
        }
        if(logBufferPtr_->capacity()-logBufferPtr_->length() < len)
        {
            writeBuffers_.push(logBufferPtr_);
            logBufferPtr_=std::make_shared<std::string>();
            logBufferPtr_->reserve(MEM_BUFFER_SIZE);
            cond_.notify_one();
        }


//        if(lostCounter_>0)
//        {
//            char logErr[128];
//            sprintf(logErr,"%ld log information is lost\n",lostCounter_);
//            lostCounter_=0;
//            logBuffer_.append(logErr);
//        }
        logBufferPtr_->append(std::string(msg,len));
    }
    void AsyncFileLogger::flush() {
        std::lock_guard<std::mutex> guard_(mutex_);
        if(logBufferPtr_->length()>0)
            cond_.notify_one();
    }
    void AsyncFileLogger::writeLogToFile(const StringPtr buf) {
        if(!loggerFilePtr_)
        {
            loggerFilePtr_=std::unique_ptr<LoggerFile>(new LoggerFile(filePath_,fileBaseName_,fileExtName_));
        }
        loggerFilePtr_->writeLog(buf);
        if(loggerFilePtr_->getLength()>sizeLimit_)
        {
            loggerFilePtr_.reset();
        }
    }
    void AsyncFileLogger::logThreadFunc() {
        prctl(PR_SET_NAME,"AsyncFileLogger");
        while (!stopFlag_) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::seconds(LOG_FLUSH_TIMEOUT), [=]() { return logBufferPtr_->length() > 0||writeBuffers_.size()>0; });
            std::string tmpStr;
            if(logBufferPtr_->length()>0)
            {
                writeBuffers_.push(logBufferPtr_);
                logBufferPtr_=std::make_shared<std::string>();
                logBufferPtr_->reserve(MEM_BUFFER_SIZE);
            }
            if(writeBuffers_.size()>0)
            {
                StringPtr tmpPtr=(StringPtr &&)writeBuffers_.front();
                writeBuffers_.pop();
                lock.unlock();
                writeLogToFile(tmpPtr);
                lock.lock();
            }
        }
    }
    void AsyncFileLogger::startLogging() {
        threadPtr_=std::unique_ptr<std::thread>(
                new std::thread(std::bind(&AsyncFileLogger::logThreadFunc, this)));
    }

    AsyncFileLogger::LoggerFile::LoggerFile(const std::string &filePath, const std::string &fileBaseName,const std::string &fileExtName)
            :createDate_(Date::date()),
             filePath_(filePath),
             fileBaseName_(fileBaseName),
             fileExtName_(fileExtName)
    {
        fileFullName_=filePath+fileBaseName+fileExtName;
        fp_=fopen(fileFullName_.c_str(),"a");
        if(fp_!=NULL)
        {

        }
        else
        {
            std::cout<<strerror(errno)<<std::endl;
        }
    }
    uint64_t AsyncFileLogger::LoggerFile::fileSeq_=0;
    void AsyncFileLogger::LoggerFile::writeLog(const StringPtr buf) {
        if(fp_)
        {
            fwrite(buf->c_str(),1,buf->length(),fp_);
        }
    }
    uint64_t AsyncFileLogger::LoggerFile::getLength() {
        if(fp_)
            return ftell(fp_);
        return 0;
    }
    AsyncFileLogger::LoggerFile::~LoggerFile()
    {
        if(fp_)
        {
            fclose(fp_);
            char seq[10];
            sprintf(seq,".%06ld",fileSeq_%1000000);
            fileSeq_++;
            std::string newName=filePath_+fileBaseName_+"."+createDate_.toCustomedFormattedString("%y%m%d-%H%M%S")+std::string(seq)
                                +fileExtName_;
            rename(fileFullName_.c_str(),newName.c_str());
        }
    }
}