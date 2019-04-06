/**
 *
 *  MsgBuffer.h
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

#include <trantor/utils/config.h>
#include <trantor/utils/NonCopyable.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define _bufferDEF_LENGTH 2048
static const char CRLF[] = "\r\n";

namespace trantor
{
class MsgBuffer
{
  public:
    MsgBuffer(size_t len = _bufferDEF_LENGTH);
    //default constructor and assignment operator can work;
    //        MsgBuffer(const MsgBuffer &buf);
    //        MsgBuffer&operator = (const MsgBuffer &buf);
    //        MsgBuffer(MsgBuffer &&buf) noexcept ;
    //        MsgBuffer& operator = (MsgBuffer &&buf) noexcept ;
    //peek
    const char *peek() const { return begin() + _head; }
    const char *beginWrite() const { return begin() + _tail; }
    char *beginWrite() { return begin() + _tail; }
    uint8_t peekInt8() const
    {
        assert(readableBytes() >= 1);
        return *(static_cast<const uint8_t *>((void *)peek()));
    }
    uint16_t peekInt16() const;
    uint32_t peekInt32() const;
    uint64_t peekInt64() const;
    //read
    std::string read(size_t len);
    uint8_t readInt8();
    uint16_t readInt16();
    uint32_t readInt32();
    uint64_t readInt64();
    void swap(MsgBuffer &buf);
    size_t readableBytes() const { return _tail - _head; }
    size_t writableBytes() const { return _buffer.size() - _tail; }
    //append
    void append(const MsgBuffer &buf);
    template <int N>
    void append(const char (&buf)[N])
    {
        assert(strnlen(buf, N) == N - 1);
        ensureWritableBytes(N - 1);
        append(buf, N - 1);
    }
    void append(const char *buf, size_t len);
    void append(const std::string &buf)
    {
        ensureWritableBytes(buf.length());
        append(buf.c_str(), buf.length());
    }
    void append(const string_view &buf)
    {
        if (buf.empty())
            return;
        ensureWritableBytes(buf.length());
        append(buf.data(), buf.length());
    }
    void appendInt8(const uint8_t b) { append(static_cast<const char *>((void *)&b), 1); }
    void appendInt16(const uint16_t s);
    void appendInt32(const uint32_t i);
    void appendInt64(const uint64_t l);
    //add in front
    void addInFront(const char *buf, size_t len);
    void addInFrontInt8(const int8_t b) { addInFront(static_cast<const char *>((void *)&b), 1); }
    void addInFrontInt16(const int16_t s);
    void addInFrontInt32(const int32_t i);
    void addInFrontInt64(const int64_t l);
    void retrieveAll();
    void retrieve(size_t len);
    ssize_t readFd(int fd, int *retErrno);
    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    //find
    const char *findCRLF() const
    {
        const char *crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    void ensureWritableBytes(size_t len);
    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        _tail += len;
    }
    //cancel
    void unwrite(size_t offset)
    {
        assert(readableBytes() >= offset);
        _tail -= offset;
    }
    const char &operator[](size_t offset) const
    {
        assert(readableBytes() >= offset);
        return peek()[offset];
    }
    char &operator[](size_t offset)
    {
        assert(readableBytes() >= offset);
        return begin()[_head + offset];
    }

  private:
    size_t _head;
    size_t _initCap;
    std::vector<char> _buffer;
    size_t _tail;
    const char *begin() const { return &_buffer[0]; }
    char *begin() { return &_buffer[0]; }
};
} // namespace trantor
