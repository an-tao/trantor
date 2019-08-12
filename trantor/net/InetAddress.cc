// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <trantor/net/InetAddress.h>

#include <trantor/utils/Logger.h>
//#include <muduo/net/Endian.h>

#include <netdb.h>
#include <strings.h>  // memset
#include <netinet/in.h>
#include <netinet/tcp.h>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace trantor;

/*
#ifdef __linux__
#if !(__GNUC_PREREQ(4, 6))
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
#endif
*/

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
    : _isIpV6(ipv6)
{
    if (ipv6)
    {
        memset(&_addr6, 0, sizeof(_addr6));
        _addr6.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        _addr6.sin6_addr = ip;
        _addr6.sin6_port = htons(port);
    }
    else
    {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        _addr.sin_addr.s_addr = htonl(ip);
        _addr.sin_port = htons(port);
    }
}

InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6)
    : _isIpV6(ipv6)
{
    if (ipv6)
    {
        memset(&_addr6, 0, sizeof(_addr6));
        _addr6.sin6_family = AF_INET6;
        _addr6.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip.c_str(), &_addr6.sin6_addr) <= 0)
        {
            // LOG_SYSERR << "sockets::fromIpPort";
            // abort();
        }
    }
    else
    {
        memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, ip.c_str(), &_addr.sin_addr) <= 0)
        {
            // LOG_SYSERR << "sockets::fromIpPort";
            // abort();
        }
    }
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = "";
    uint16_t port = ntohs(_addr.sin_port);
    sprintf(buf, ":%u", port);
    return toIp() + std::string(buf);
}
bool InetAddress::isIntranetIp() const
{
    if (_addr.sin_family == AF_INET)
    {
        uint32_t ip_addr = ntohl(_addr.sin_addr.s_addr);
        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
            (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
            ip_addr == 0x7f000001)

        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        // Loopback ip
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        // Privated ip is prefixed by FEC0::/10 or FE80::/10, need testing
        auto i32 = (ntohl(*addrP) & 0xffc00000);
        if (i32 == 0xfec00000 || i32 == 0xfe800000)
            return true;
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff)
        {
            // the IPv6 version of an IPv4 IP address
            uint32_t ip_addr = ntohl(*(addrP + 3));
            if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
                (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
                (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
                ip_addr == 0x7f000001)

            {
                return true;
            }
        }
    }
    return false;
}

bool InetAddress::isLoopbackIp() const
{
    if (!isIpV6())
    {
        uint32_t ip_addr = ntohl(_addr.sin_addr.s_addr);
        if (ip_addr == 0x7f000001)
        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        // the IPv6 version of an IPv4 loopback address
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff &&
            ntohl(*(addrP + 3)) == 0x7f000001)
            return true;
    }
    return false;
}

std::string InetAddress::toIp() const
{
    char buf[64];
    if (_addr.sin_family == AF_INET)
    {
        ::inet_ntop(AF_INET, &_addr.sin_addr, buf, sizeof(buf));
    }
    else if (_addr.sin_family == AF_INET6)
    {
        ::inet_ntop(AF_INET6, &_addr6.sin6_addr, buf, sizeof(buf));
    }

    return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
    // assert(family() == AF_INET);
    return _addr.sin_addr.s_addr;
}

const uint32_t *InetAddress::ip6NetEndian() const
{
// assert(family() == AF_INET6);
#ifdef __linux__
    return _addr6.sin6_addr.s6_addr32;
#else
    return _addr6.sin6_addr.__u6_addr.__u6_addr32;
#endif
}
uint16_t InetAddress::toPort() const
{
    return ntohs(portNetEndian());
}
