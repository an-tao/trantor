#pragma once

#include <netinet/in.h>
namespace trantor
{
    inline uint64_t hton64(uint64_t n) {
        return (((uint64_t)htonl(n)) << 32) | htonl(n >> 32);
    }
    inline uint64_t ntoh64(uint64_t n) {
        return (((uint64_t)ntohl(n)) << 32) | ntohl(n >> 32);
    }
}
