/**
 *
 *  WindowsSupport.h
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

#include <stdint.h>

struct iovec
{
    void *iov_base; /* Starting address */
    size_t iov_len; /* Number of bytes */
};

int readv(int fd, const struct iovec *vector, int count);
