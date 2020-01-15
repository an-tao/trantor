// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <trantor/utils/Date.h>

#ifdef _WIN32
#include <ws2tcpip.h>
using sa_family_t = unsigned short;
using in_addr_t = uint32_t;
using uint16_t = unsigned short;
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <string>
#include <unordered_map>
#include <mutex>
namespace trantor
{
// namespace sockets
//{
// const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
//}

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress
{
  public:
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    InetAddress(uint16_t port = 0,
                bool loopbackOnly = false,
                bool ipv6 = false);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    explicit InetAddress(const struct sockaddr_in &addr) : addr_(addr)
    {
    }

    explicit InetAddress(const struct sockaddr_in6 &addr)
        : addr6_(addr), isIpV6_(true)
    {
    }

    sa_family_t family() const
    {
        return addr_.sin_family;
    }
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    bool isIpV6() const
    {
        return isIpV6_;
    }
    // default copy/assignment are Okay

    bool isIntranetIp() const;
    bool isLoopbackIp() const;

    const struct sockaddr *getSockAddr() const
    {
        return static_cast<const struct sockaddr *>((void *)(&addr6_));
    }
    void setSockAddrInet6(const struct sockaddr_in6 &addr6)
    {
        addr6_ = addr6;
        isIpV6_ = (addr6_.sin6_family == AF_INET6);
    }

    uint32_t ipNetEndian() const;
    const uint32_t *ip6NetEndian() const;
    uint16_t portNetEndian() const
    {
        return addr_.sin_port;
    }
    void setPortNetEndian(uint16_t port)
    {
        addr_.sin_port = port;
    }

  private:
    union {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
    bool isIpV6_{false};
};

}  // namespace trantor

#endif  // MUDUO_NET_INETADDRESS_H
