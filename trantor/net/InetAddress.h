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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <unordered_map>
#include <mutex>
namespace trantor
{
//namespace sockets
//{
//const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
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
    InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    explicit InetAddress(const struct sockaddr_in &addr)
        : _addr(addr)
    {
    }

    explicit InetAddress(const struct sockaddr_in6 &addr)
        : _addr6(addr), _isIpV6(true)
    {
    }

    sa_family_t family() const { return _addr.sin_family; }
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    bool isIpV6() const { return _isIpV6; }
    // default copy/assignment are Okay

    bool isIntranetIp() const;
    bool isLoopbackIp() const;

    const struct sockaddr *getSockAddr() const
    {
        return static_cast<const struct sockaddr *>((void *)(&_addr6));
    }
    void setSockAddrInet6(const struct sockaddr_in6 &addr6)
    {
        _addr6 = addr6;
        _isIpV6 = (_addr6.sin6_family == AF_INET6);
    }

    uint32_t ipNetEndian() const;
    const uint32_t *ip6NetEndian() const;
    uint16_t portNetEndian() const { return _addr.sin_port; }

    // resolve hostname to IP address, not changing port or sin_family
    // return true on success.
    // thread safe
    static bool resolve(const std::string &hostname, InetAddress *result, size_t timeout = 3600);
    // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

  private:
    union {
        struct sockaddr_in _addr;
        struct sockaddr_in6 _addr6;
    };
    bool _isIpV6 = false;
    static std::unordered_map<std::string, std::pair<struct in_addr, trantor::Date>> _dnsCache;
    static std::mutex _dnsMutex;
};

} // namespace trantor

#endif // MUDUO_NET_INETADDRESS_H
