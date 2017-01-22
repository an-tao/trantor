#include <trantor/utils/AsyncFileLogger.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#define LOG_FLUSH_TIMEOUT 1
#define MEM_BUFFER_SIZE 1024*1024
namespace trantor {
    AsyncFileLogger::AsyncFileLogger()
    {
        logBuffer_.reserve(MEM_BUFFER_SIZE*3);
        writeBuffer_.reserve(MEM_BUFFER_SIZE*3);
    }
    AsyncFileLogger::~AsyncFileLogger()
    {
        std::cout<<"~AsyncFileLogger:buffer len="<<logBuffer_.length()<<std::endl;
        stopFlag_=true;
        if(threadPtr_)
        {
            stopFlag_=true;
            cond_.notify_one();
            threadPtr_->join();
        }
        std::lock_guard<std::mutex> guard_(mutex_);
        if(logBuffer_.length()>0)
            writeLogToFile(logBuffer_);
    }
    void AsyncFileLogger::output(const std::stringstream &out) {

        std::lock_guard<std::mutex> guard_(mutex_);
        if(logBuffer_.length() > MEM_BUFFER_SIZE*2)
        {
            lostCounter_++;
            return;
        }
        if(lostCounter_>0)
        {
            char logErr[128];
            sprintf(logErr,"%ld log information is lost\n",lostCounter_);
            lostCounter_=0;
            logBuffer_.append(logErr);
        }
        logBuffer_.append(out.str());
        if (logBuffer_.length() > MEM_BUFFER_SIZE) {
            cond_.notify_one();
        }
    }
    void AsyncFileLogger::flush() {
        std::lock_guard<std::mutex> guard_(mutex_);
        if(logBuffer_.length()>0)
            cond_.notify_one();
    }
    void AsyncFileLogger::writeLogToFile(const std::string &buf) {
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
        while (!stopFlag_) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::seconds(LOG_FLUSH_TIMEOUT), [=]() { return logBuffer_.length() > 0; });
            std::string tmpStr;
            writeBuffer_.clear();
            //std::cout<<"writeBuffer.capacity:"<<writeBuffer_.capacity()<<std::endl;
            writeBuffer_.swap(logBuffer_);
            lock.unlock();
            writeLogToFile(writeBuffer_);
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
    void AsyncFileLogger::LoggerFile::writeLog(const std::string &buf) {
        if(fp_)
        {
            fwrite(buf.c_str(),1,buf.length(),fp_);
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
            std::string newName=filePath_+fileBaseName_+"."+createDate_.toCustomedFormattedString("%y%m%d-%H%M%S")
                                +fileExtName_;
            rename(fileFullName_.c_str(),newName.c_str());
        }
    }
}