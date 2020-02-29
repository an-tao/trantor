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
#include <algorithm>
namespace trantor
{
inline uint64_t hton64(uint64_t n)
{
    static const int one = 1;
    static const char sig = *(char*)&one;
    if (sig == 0)
        return n;  // for big endian machine just return the input
    char* ptr = reinterpret_cast<char*>(&n);
    std::reverse(ptr, ptr + sizeof(uint64_t));
    return n;
}
inline uint64_t ntoh64(uint64_t n)
{
    return hton64(n);
}
}  // namespace trantor
