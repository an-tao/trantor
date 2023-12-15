/**
 *
 *  @file BufferNode.h
 *  @author An Tao
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
#ifdef _WIN32
#include <stdio.h>
#endif
#include <trantor/utils/MsgBuffer.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/Logger.h>
#include <functional>
#include <memory>
#include <string>

namespace trantor
{
class BufferNode;
using BufferNodePtr = std::shared_ptr<BufferNode>;
using StreamCallback = std::function<std::size_t(char *, std::size_t)>;
class BufferNode : public NonCopyable
{
  public:
    virtual bool isFile() const
    {
        return false;
    }
    virtual ~BufferNode() = default;
    virtual bool isStream() const
    {
        return false;
    }
    virtual void getData(const char *&data, size_t &len) = 0;
    virtual void append(const char *, size_t)
    {
        LOG_FATAL << "Not a memory buffer node";
    }
    virtual void retrieve(size_t len) = 0;
    virtual size_t remainingBytes() const = 0;
    virtual int getFd() const
    {
        LOG_FATAL << "Not a file buffer node";
        return -1;
    }
    void done()
    {
        isDone_ = true;
    }
    static BufferNodePtr newMemBufferNode();

    static BufferNodePtr newStreamBufferNode(StreamCallback &&cb);
#ifdef _WIN32
    static BufferNodePtr newFileBufferNode(FILE *fp,
                                           long long offset,
                                           size_t length);
#else
    static BufferNodePtr newFileBufferNode(int fd, off_t offset, size_t length);
#endif
  protected:
    bool isDone_{false};
};

}  // namespace trantor