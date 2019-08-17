/**
 *
 *  MsgBuffer.cc
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

#include <trantor/utils/MsgBuffer.h>
#include <trantor/utils/Funcs.h>
#include <string.h>
#include <sys/uio.h>
#include <errno.h>
#include <netinet/in.h>
#include <assert.h>

using namespace trantor;
#define BUF_OFFSET 8
MsgBuffer::MsgBuffer(size_t len)
    : _head(BUF_OFFSET), _initCap(len), _buffer(len + _head), _tail(_head)
{
}
// MsgBuffer::MsgBuffer(const MsgBuffer &buf)
//        :_head(buf._head),
//         _initCap(buf._initCap),
//         _buffer(buf._buffer),
//         _tail(buf._tail)
//{
//
//}
// MsgBuffer& MsgBuffer::operator = (const MsgBuffer &buf)
//{
//    if(this!=&buf)
//    {
//        _head=buf._head;
//        _initCap=buf._initCap;
//        _buffer=buf._buffer;
//        _tail=buf._tail;
//    }
//
//    return *this;
//}
// MsgBuffer::MsgBuffer(MsgBuffer &&buf) noexcept
//:_head(buf._head),
// _initCap(buf._initCap),
// _buffer(std::move(buf._buffer)),
// _tail(buf._tail)
//{
//
//}
// MsgBuffer& MsgBuffer::operator = (MsgBuffer &&buf) noexcept
//{
//    if(this!=&buf)
//    {
//        _head=buf._head;
//        _initCap=buf._initCap;
//        _buffer=std::move(buf._buffer);
//        _tail=buf._tail;
//    }
//
//    return *this;
//}
void MsgBuffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() >= len)
        return;
    if (_head + writableBytes() >= (len + BUF_OFFSET))  // move readable bytes
    {
        std::copy(begin() + _head, begin() + _tail, begin() + BUF_OFFSET);
        _tail = BUF_OFFSET + (_tail - _head);
        _head = BUF_OFFSET;
        return;
    }
    // create new buffer
    size_t newLen;
    if ((_buffer.size() * 2) > (BUF_OFFSET + readableBytes() + len))
        newLen = _buffer.size() * 2;
    else
        newLen = BUF_OFFSET + readableBytes() + len;
    MsgBuffer newbuffer(newLen);
    newbuffer.append(*this);
    swap(newbuffer);
}
void MsgBuffer::swap(MsgBuffer &buf) noexcept
{
    _buffer.swap(buf._buffer);
    std::swap(_head, buf._head);
    std::swap(_tail, buf._tail);
    std::swap(_initCap, buf._initCap);
}
void MsgBuffer::append(const MsgBuffer &buf)
{
    ensureWritableBytes(buf.readableBytes());
    memcpy(&_buffer[_tail], buf.peek(), buf.readableBytes());
    _tail += buf.readableBytes();
}
void MsgBuffer::append(const char *buf, size_t len)
{
    ensureWritableBytes(len);
    memcpy(&_buffer[_tail], buf, len);
    _tail += len;
}
void MsgBuffer::appendInt16(const uint16_t s)
{
    uint16_t ss = htons(s);
    append(static_cast<const char *>((void *)&ss), 2);
}
void MsgBuffer::appendInt32(const uint32_t i)
{
    uint32_t ii = htonl(i);
    append(static_cast<const char *>((void *)&ii), 4);
}
void MsgBuffer::appendInt64(const uint64_t l)
{
    uint64_t ll = hton64(l);
    append(static_cast<const char *>((void *)&ll), 8);
}

void MsgBuffer::addInFrontInt16(const int16_t s)
{
    uint16_t ss = htons(s);
    addInFront(static_cast<const char *>((void *)&ss), 2);
}
void MsgBuffer::addInFrontInt32(const int32_t i)
{
    uint32_t ii = htonl(i);
    addInFront(static_cast<const char *>((void *)&ii), 4);
}
void MsgBuffer::addInFrontInt64(const int64_t l)
{
    uint64_t ll = hton64(l);
    addInFront(static_cast<const char *>((void *)&ll), 8);
}

uint16_t MsgBuffer::peekInt16() const
{
    assert(readableBytes() >= 2);
    uint16_t rs = *(static_cast<const uint16_t *>((void *)peek()));
    return ntohs(rs);
}
uint32_t MsgBuffer::peekInt32() const
{
    assert(readableBytes() >= 4);
    uint32_t rl = *(static_cast<const uint32_t *>((void *)peek()));
    return ntohl(rl);
}
uint64_t MsgBuffer::peekInt64() const
{
    assert(readableBytes() >= 8);
    uint64_t rll = *(static_cast<const uint64_t *>((void *)peek()));
    return ntoh64(rll);
}

void MsgBuffer::retrieve(size_t len)
{
    if (len >= readableBytes())
    {
        retrieveAll();
        return;
    }
    _head += len;
}
void MsgBuffer::retrieveAll()
{
    if (_buffer.size() > (_initCap * 2))
    {
        _buffer.resize(_initCap);
    }
    _tail = _head = BUF_OFFSET;
}
ssize_t MsgBuffer::readFd(int fd, int *retErrno)
{
    char extBuffer[65536];
    struct iovec vec[2];
    size_t writable = writableBytes();
    vec[0].iov_base = begin() + _tail;
    vec[0].iov_len = writable;
    vec[1].iov_base = extBuffer;
    vec[1].iov_len = sizeof(extBuffer);
    const int iovcnt = (writable < sizeof extBuffer) ? 2 : 1;
    ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *retErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        _tail += n;
    }
    else
    {
        _tail = _buffer.size();
        append(extBuffer, n - writable);
    }
    return n;
}

std::string MsgBuffer::read(size_t len)
{
    if (len > readableBytes())
        len = readableBytes();
    std::string ret(peek(), len);
    retrieve(len);
    return ret;
}
uint8_t MsgBuffer::readInt8()
{
    uint8_t ret = peekInt8();
    retrieve(1);
    return ret;
}
uint16_t MsgBuffer::readInt16()
{
    uint16_t ret = peekInt16();
    retrieve(2);
    return ret;
}
uint32_t MsgBuffer::readInt32()
{
    uint32_t ret = peekInt32();
    retrieve(4);
    return ret;
}
uint64_t MsgBuffer::readInt64()
{
    uint64_t ret = peekInt64();
    retrieve(8);
    return ret;
}

void MsgBuffer::addInFront(const char *buf, size_t len)
{
    if (_head >= len)
    {
        memcpy(begin() + _head - len, buf, len);
        _head -= len;
        return;
    }
    if (len <= writableBytes())
    {
        std::copy(begin() + _head, begin() + _tail, begin() + _head + len);
        memcpy(begin() + _head, buf, len);
        _tail += len;
        return;
    }
    size_t newLen;
    if (len + readableBytes() < _initCap)
        newLen = _initCap;
    else
        newLen = len + readableBytes();
    MsgBuffer newBuf(newLen);
    newBuf.append(buf, len);
    newBuf.append(*this);
    swap(newBuf);
}
