/**
 *
 *  @file AsyncStream.h
 *  @author An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2023, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#pragma once

#include <trantor/utils/NonCopyable.h>
#include <memory>

namespace trantor
{
/**
 * @brief This class represents a data stream that can be sent asynchronously.
 * The data is sent in chunks, and the chunks are sent in order, and all the
 * chunks are sent continuously.
 */
class TRANTOR_EXPORT AsyncStream : public NonCopyable
{
  public:
    virtual ~AsyncStream() = default;
    virtual void send(const char *msg, size_t len) = 0;
    virtual void close() = 0;
};
using AsyncStreamPtr = std::unique_ptr<AsyncStream>;
}  // namespace trantor