/**
 *
 *  Funcs.h
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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif
namespace trantor
{
inline uint64_t hton64(uint64_t n)
{
    return (((uint64_t)htonl(static_cast<u_long>(n))) << 32) |
           htonl(static_cast<u_long>(n >> 32));
}
inline uint64_t ntoh64(uint64_t n)
{
    return hton64(n);
}
}  // namespace trantor
