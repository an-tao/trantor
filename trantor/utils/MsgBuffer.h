#pragma once

#include <trantor/utils/NonCopyable.h>
#include <vector>
#include <stdio.h>
namespace trantor
{
    class MsgBuffer
    {
    public:
        MsgBuffer(size_t len=2048);
        const char *peek() const {return begin()+head_;}

        void swap(MsgBuffer &buf);
        const size_t readableBytes() const {return tail_-head_;}
        const size_t writableBytes() const {return buffer_.size()-tail_;}
        void ensureWritableBytes(size_t len);
        void append(const MsgBuffer &buf);
        void append(const char *buf,size_t len);
        void retrieveAll();
        void retrieve(size_t len);
        size_t readFd(int fd,int *retErrno);
    private:
        size_t head_;
        std::vector<char> buffer_;

        size_t tail_;
        const char *begin() const {return &buffer_[0];}
        char *begin() {return &buffer_[0];}
    };
}