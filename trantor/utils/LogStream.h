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

// token from muduo lib
#include <trantor/utils/config.h>
#include <trantor/utils/NonCopyable.h>

#include <assert.h>
#include <string.h>  // memcpy
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
    FixedBuffer() : _cur(_data)
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
            memcpy(_cur, buf, len);
            _cur += len;
            return true;
        }
        return false;
    }

    const char *data() const
    {
        return _data;
    }
    int length() const
    {
        return static_cast<int>(_cur - _data);
    }

    // write to _data directly
    char *current()
    {
        return _cur;
    }
    int avail() const
    {
        return static_cast<int>(end() - _cur);
    }
    void add(size_t len)
    {
        _cur += len;
    }

    void reset()
    {
        _cur = _data;
    }
    void bzero()
    {
        memset(_data, 0, sizeof(_data));
    }

    // for used by GDB
    const char *debugString();
    void setCookie(void (*cookie)())
    {
        cookie_ = cookie;
    }
    // for used by unit test
    std::string toString() const
    {
        return std::string(_data, length());
    }
    // StringPiece toStringPiece() const { return StringPiece(_data, length());
    // }

  private:
    const char *end() const
    {
        return _data + sizeof _data;
    }
    // Must be outline function for cookies.
    static void cookieStart();
    static void cookieEnd();

    void (*cookie_)();
    char _data[SIZE];
    char *_cur;
};

}  // namespace detail

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
    template <int N>
    self &operator<<(const char (&buf)[N])
    {
        assert(strnlen(buf, N) == N - 1);
        append(buf, N - 1);
        return *this;
    }

    self &operator<<(char *str)
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

    self &operator<<(const string_view &buf)
    {
        if (!buf.empty())
        {
            append(buf.data(), buf.length());
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

    void append(const char *data, int len)
    {
        if (_exBuffer.empty())
        {
            if (!_buffer.append(data, len))
            {
                _exBuffer.append(_buffer.data(), _buffer.length());
                _exBuffer.append(data, len);
            }
        }
        else
        {
            _exBuffer.append(data, len);
        }
    }
    // const Buffer& buffer() const { return _buffer; }
    const char *bufferData() const
    {
        if (!_exBuffer.empty())
        {
            return _exBuffer.data();
        }
        return _buffer.data();
    }

    size_t bufferLength() const
    {
        if (!_exBuffer.empty())
        {
            return _exBuffer.length();
        }
        return _buffer.length();
    }
    void resetBuffer()
    {
        _buffer.reset();
        _exBuffer.clear();
    }

  private:
    void staticCheck();

    template <typename T>
    void formatInteger(T);

    Buffer _buffer;
    std::string _exBuffer;
    static const int kMaxNumericSize = 32;
};

class Fmt  // : boost::noncopyable
{
  public:
    template <typename T>
    Fmt(const char *fmt, T val);

    const char *data() const
    {
        return _buf;
    }
    int length() const
    {
        return _length;
    }

  private:
    char _buf[32];
    int _length;
};

inline LogStream &operator<<(LogStream &s, const Fmt &fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

}  // namespace trantor
