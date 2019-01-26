/**
 *
 *  LogStream.h
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

//token from muduo lib

#include <trantor/utils/NonCopyable.h>

#include <assert.h>
#include <string.h> // memcpy
#include <string>

namespace trantor
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : NonCopyable
{
  public:
    FixedBuffer()
        : cur_(data_)
    {
        setCookie(cookieStart);
    }

    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    bool append(const char * /*restrict*/ buf, size_t len)
    {
        if ((size_t)(avail()) > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
            return true;
        }
        return false;
    }

    const char *data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }

    // write to data_ directly
    char *current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { ::bzero(data_, sizeof data_); }

    // for used by GDB
    const char *debugString();
    void setCookie(void (*cookie)()) { cookie_ = cookie; }
    // for used by unit test
    std::string toString() const { return std::string(data_, length()); }
    //StringPiece toStringPiece() const { return StringPiece(data_, length()); }

  private:
    const char *end() const { return data_ + sizeof data_; }
    // Must be outline function for cookies.
    static void cookieStart();
    static void cookieEnd();

    void (*cookie_)();
    char data_[SIZE];
    char *cur_;
};

} // namespace detail

class LogStream : NonCopyable
{
    typedef LogStream self;

  public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

    self &operator<<(bool v)
    {
        append(v ? "1" : "0", 1);
        return *this;
    }

    self &operator<<(short);
    self &operator<<(unsigned short);
    self &operator<<(int);
    self &operator<<(unsigned int);
    self &operator<<(long);
    self &operator<<(unsigned long);
    self &operator<<(long long);
    self &operator<<(unsigned long long);

    self &operator<<(const void *);

    self &operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    self &operator<<(double);
    // self& operator<<(long double);

    self &operator<<(char v)
    {
        append(&v, 1);
        return *this;
    }

    // self& operator<<(signed char);
    // self& operator<<(unsigned char);

    self &operator<<(const char *str)
    {
        if (str)
        {
            append(str, strlen(str));
        }
        else
        {
            append("(null)", 6);
        }
        return *this;
    }

    self &operator<<(const unsigned char *str)
    {
        return operator<<(reinterpret_cast<const char *>(str));
    }

    self &operator<<(const std::string &v)
    {
        append(v.c_str(), v.size());
        return *this;
    }

    //  self& operator<<(const StringPiece& v)
    //  {
    //    buffer_.append(v.data(), v.size());
    //    return *this;
    //  }

    //  self& operator<<(const Buffer& v)
    //  {
    //    *this << v.toStringPiece();
    //    return *this;
    //  }

    void append(const char *data, int len)
    {
        if (_exBuffer.empty())
        {
            if (!buffer_.append(data, len))
            {
                _exBuffer.append(buffer_.data(), buffer_.length());
                _exBuffer.append(data, len);
            }
        }
        else
        {
            _exBuffer.append(data, len);
        }
    }
    //const Buffer& buffer() const { return buffer_; }
    const char *bufferData() const
    {
        if (!_exBuffer.empty())
        {
            return _exBuffer.data();
        }
        return buffer_.data();
    }
    size_t bufferLength() const
    {
        if (!_exBuffer.empty())
        {
            return _exBuffer.length();
        }
        return buffer_.length();
    }
    void resetBuffer()
    {
        buffer_.reset();
        _exBuffer.clear();
    }

  private:
    void staticCheck();

    template <typename T>
    void formatInteger(T);

    Buffer buffer_;
    std::string _exBuffer;
    static const int kMaxNumericSize = 32;
};

class Fmt // : boost::noncopyable
{
  public:
    template <typename T>
    Fmt(const char *fmt, T val);

    const char *data() const { return buf_; }
    int length() const { return length_; }

  private:
    char buf_[32];
    int length_;
};

inline LogStream &operator<<(LogStream &s, const Fmt &fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

} // namespace trantor
