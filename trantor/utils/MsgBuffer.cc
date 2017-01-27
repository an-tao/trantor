#include <trantor/utils/MsgBuffer.h>
#include <string.h>
#include <sys/uio.h>
#include <errno.h>
using namespace trantor;
#define BUF_OFFSET 8
MsgBuffer::MsgBuffer(size_t len)
        :head_(BUF_OFFSET),
         buffer_(len+head_),
         tail_(head_)
{
}
void MsgBuffer::ensureWritableBytes(size_t len)
{
    if(writableBytes()>=len)
        return;
    if(head_+writableBytes()>=(len+BUF_OFFSET))//move readable bytes
    {
        std::copy(begin()+head_,begin()+tail_,begin()+BUF_OFFSET);
        tail_=BUF_OFFSET+(tail_-head_);
        head_=BUF_OFFSET;
        return;
    }
    //create new buffer
    MsgBuffer newbuffer(BUF_OFFSET+readableBytes()+len);
    newbuffer.append(*this);
    swap(newbuffer);
}
void MsgBuffer::swap(MsgBuffer &buf) {
    buffer_.swap(buf.buffer_);
    std::swap(head_,buf.head_);
    std::swap(tail_,buf.tail_);
}
void MsgBuffer::append(const MsgBuffer &buf) {
    ensureWritableBytes(buf.readableBytes());
    memcpy(&buffer_[tail_],buf.peek(),buf.readableBytes());
    tail_+=buf.readableBytes();
}
void MsgBuffer::append(const char *buf, size_t len) {
    ensureWritableBytes(len);
    memcpy(&buffer_[tail_],buf,len);
    tail_ +=len;
}
void MsgBuffer::retrieve(size_t len) {
    if(len>=readableBytes())
    {
        retrieveAll();
        return;
    }
    head_+=len;
}
void MsgBuffer::retrieveAll() {
    tail_=head_=BUF_OFFSET;
}
size_t MsgBuffer::readFd(int fd,int *retErrno){
    char extBuffer[65536];
    struct iovec vec[2];
    size_t writable=writableBytes();
    vec[0].iov_base=begin()+tail_;
    vec[0].iov_len=writable;
    vec[1].iov_base=extBuffer;
    vec[1].iov_len=sizeof(extBuffer);
    const int iovcnt = (writable < sizeof extBuffer) ? 2 : 1;
    ssize_t n=::readv(fd,vec,iovcnt);
    if(n<0)
    {
        *retErrno=errno;
    }
    else if(static_cast<size_t>(n) <= writable)
    {
        tail_+=n;
    }else
    {
        tail_=buffer_.size();
        append(extBuffer,n-writable);
    }
    return n;
}
