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
#include <strings.h> // memset
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

#ifdef __linux__
#if !(__GNUC_PREREQ(4, 6))
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
#endif
InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
    : _isIpV6(ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port);
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htonl(ip);
        addr_.sin_port = htons(port);
    }
}

InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6) : _isIpV6(ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0)
        {
            LOG_SYSERR << "sockets::fromIpPort";
        }
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
        {
            LOG_SYSERR << "sockets::fromIpPort";
        }
    }
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = "";
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf, ":%u", port);
    return toIp() + std::string(buf);
}
bool InetAddress::isInnerIp() const
{
    if (addr_.sin_family == AF_INET)
    {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
            (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
            ip_addr == 0x7f000001)

        {
            return true;
        }
    }
    return false;
}
std::string InetAddress::toIp() const
{
    char buf[64];
    if (addr_.sin_family == AF_INET)
    {
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    }
    else if (addr_.sin_family == AF_INET6)
    {
        ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf));
    }

    return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(portNetEndian());
}
#ifdef __linux__
static __thread char t_resolveBuffer[64 * 1024];
#endif
std::mutex InetAddress::_dnsMutex;
std::unordered_map<std::string, std::pair<struct in_addr, trantor::Date>> InetAddress::_dnsCache;
bool InetAddress::resolve(const std::string &hostname, InetAddress *out, size_t timeout)
{
    assert(out != NULL);
    {
        std::lock_guard<std::mutex> guard(_dnsMutex);
        if (_dnsCache.find(hostname) != _dnsCache.end())
        {
            auto &addr = _dnsCache[hostname];
            if (timeout == 0 || (timeout > 0 &&
                                 (addr.second.after(timeout) > trantor::Date::date())))
            {
                LOG_TRACE << "dns:Hit cache";
                out->addr_.sin_addr = addr.first;
                return true;
            }
        }
    }
#ifdef __linux__
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if (ret == 0 && he != NULL)
#else
    /// Multi-threads safety
    static std::mutex _mutex;
    struct hostent *he = NULL;
    struct hostent hent;
    {
        std::lock_guard<std::mutex> guard(_mutex);
        auto result = gethostbyname(hostname.c_str());
        if (result != NULL)
        {
            memcpy(&hent, result, sizeof(hent));
            he = &hent;
        }
    }

    if (he != NULL)
#endif
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
        {
            std::lock_guard<std::mutex> guard(_dnsMutex);
            _dnsCache[hostname].first = out->addr_.sin_addr;
            _dnsCache[hostname].second = trantor::Date::date();
        }
        return true;
    }
    else
    {
        //        if (ret)
        {
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}
