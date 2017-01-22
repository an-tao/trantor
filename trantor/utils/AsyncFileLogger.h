#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/Date.h>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <sstream>
#include <memory>

namespace trantor
{
    class AsyncFileLogger:NonCopyable
    {
    public:
        //static AsyncFileLogger* getInstance();
        void output(const std::stringstream &out);
        void flush();
        void startLogging();
        void setFileSizeLimit(uint64_t limit){sizeLimit_=limit;}
        void setFileName(const std::string &baseName,const std::string &extName=".log",const std::string &path="./")
        {
            fileBaseName_=baseName;
            extName[0]=='.'?fileExtName_=extName:fileExtName_=std::string(".")+extName;
            filePath_=path;
            if(filePath_.length()==0)
                filePath_="./";
            if(filePath_[filePath_.length()-1]=='/')
                filePath_=filePath_+"/";
        }
        ~AsyncFileLogger();
        AsyncFileLogger();
    protected:
        std::mutex mutex_;
        std::condition_variable cond_;
        std::string logBuffer_;
        std::string writeBuffer_;
        void writeLogToFile(const std::string &buf);
        std::unique_ptr<std::thread> threadPtr_;
        bool stopFlag_=false;
        void logThreadFunc();
        std::string filePath_="./";
        std::string fileBaseName_="trantor";
        std::string fileExtName_=".log";
        uint64_t sizeLimit_=20*1024*1024;
        class LoggerFile:NonCopyable
        {
        public:
            LoggerFile(const std::string &filePath,const std::string &fileBaseName,const std::string &fileExtName);
            ~LoggerFile();
            void writeLog(const std::string &buf);
            uint64_t getLength();
            explicit operator bool() const { return fp_!=NULL; }
        protected:
            FILE *fp_=NULL;
            Date createDate_;
            std::string fileFullName_;
            std::string filePath_;
            std::string fileBaseName_;
            std::string fileExtName_;
        };
        std::unique_ptr<LoggerFile>loggerFilePtr_;
        uint64_t lostCounter_=0;
    };
}