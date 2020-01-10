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
    return (((uint64_t)htonl(n)) << 32) | htonl(n >> 32);
}
inline uint64_t ntoh64(uint64_t n)
{
    return (((uint64_t)ntohl(n)) << 32) | ntohl(n >> 32);
}
}  // namespace trantor
