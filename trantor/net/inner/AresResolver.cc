// Copyright 2016, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#include "AresResolver.h"
#include <trantor/net/inner/Channel.h>
#include <ares.h>
#include <netdb.h>
#include <arpa/inet.h>  // inet_ntop
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace trantor;
using namespace std::placeholders;

namespace
{
double getSeconds(struct timeval* tv)
{
    if (tv)
        return double(tv->tv_sec) + double(tv->tv_usec) / 1000000.0;
    else
        return -1.0;
}

const char* getSocketType(int type)
{
    if (type == SOCK_DGRAM)
        return "UDP";
    else if (type == SOCK_STREAM)
        return "TCP";
    else
        return "Unknown";
}

}  // namespace

std::shared_ptr<Resolver> Resolver::newResolver(trantor::EventLoop* loop,
                                                size_t timeout)
{
    return std::make_shared<AresResolver>(loop, timeout);
}

AresResolver::AresResolver(EventLoop* loop, size_t timeout)
    : _loop(loop), _ctx(NULL), _timerActive(false), _timeout(timeout)
{
    assert(_loop);
    static char lookups[] = "b";
    struct ares_options options;
    int optmask = ARES_OPT_FLAGS;
    options.flags = ARES_FLAG_NOCHECKRESP;
    options.flags |= ARES_FLAG_STAYOPEN;
    options.flags |= ARES_FLAG_IGNTC;  // UDP only
    optmask |= ARES_OPT_SOCK_STATE_CB;
    options.sock_state_cb = &AresResolver::ares_sock_state_callback;
    options.sock_state_cb_data = this;
    optmask |= ARES_OPT_TIMEOUT;
    options.timeout = 2;
    optmask |= ARES_OPT_LOOKUPS;
    options.lookups = lookups;

    int status = ares_init_options(&_ctx, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        assert(0);
    }
    ares_set_socket_callback(_ctx,
                             &AresResolver::ares_sock_create_callback,
                             this);
}

AresResolver::~AresResolver()
{
    ares_destroy(_ctx);
}

void AresResolver::resolveInLoop(const std::string& hostname,
                                 const Callback& cb)
{
    _loop->assertInLoopThread();
    auto iter = _dnsCache.find(hostname);
    if (iter != _dnsCache.end())
    {
        auto& cachedAddr = iter->second;
        if (_timeout == 0 ||
            cachedAddr.second.after(_timeout) > trantor::Date::date())
        {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof addr);
            addr.sin_family = AF_INET;
            addr.sin_port = 0;
            addr.sin_addr = cachedAddr.first;
            InetAddress inet(addr);
            cb(inet);
            return;
        }
    }
    QueryData* queryData = new QueryData(this, cb, hostname);
    ares_gethostbyname(_ctx,
                       hostname.c_str(),
                       AF_INET,
                       &AresResolver::ares_host_callback,
                       queryData);
    struct timeval tv;
    struct timeval* tvp = ares_timeout(_ctx, NULL, &tv);
    double timeout = getSeconds(tvp);
    // LOG_DEBUG << "timeout " << timeout << " active " << _timerActive;
    if (!_timerActive)
    {
        _loop->runAfter(timeout, std::bind(&AresResolver::onTimer, this));
        _timerActive = true;
    }
    return;
}

void AresResolver::onRead(int sockfd)
{
    ares_process_fd(_ctx, sockfd, ARES_SOCKET_BAD);
}

void AresResolver::onTimer()
{
    assert(_timerActive == true);
    ares_process_fd(_ctx, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    struct timeval tv;
    struct timeval* tvp = ares_timeout(_ctx, NULL, &tv);
    double timeout = getSeconds(tvp);

    if (timeout < 0)
    {
        _timerActive = false;
    }
    else
    {
        _loop->runAfter(timeout, std::bind(&AresResolver::onTimer, this));
    }
}

void AresResolver::onQueryResult(int status,
                                 struct hostent* result,
                                 const std::string& hostname,
                                 const Callback& callback)
{
    // LOG_DEBUG << "onQueryResult " << status;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    if (result)
    {
        addr.sin_addr = *reinterpret_cast<in_addr*>(result->h_addr);
    }
    InetAddress inet(addr);
    _dnsCache[hostname].first = addr.sin_addr;
    _dnsCache[hostname].second = trantor::Date::date();
    callback(inet);
}

void AresResolver::onSockCreate(int sockfd, int type)
{
    _loop->assertInLoopThread();
    assert(_channels.find(sockfd) == _channels.end());
    Channel* channel = new Channel(_loop, sockfd);
    channel->setReadCallback(std::bind(&AresResolver::onRead, this, sockfd));
    channel->enableReading();
    _channels[sockfd].reset(channel);
}

void AresResolver::onSockStateChange(int sockfd, bool read, bool write)
{
    _loop->assertInLoopThread();
    ChannelList::iterator it = _channels.find(sockfd);
    assert(it != _channels.end());
    if (read)
    {
        // update
        // if (write) { } else { }
    }
    else
    {
        // remove
        it->second->disableAll();
        it->second->remove();
        _channels.erase(it);
    }
}

void AresResolver::ares_host_callback(void* data,
                                      int status,
                                      int timeouts,
                                      struct hostent* hostent)
{
    QueryData* query = static_cast<QueryData*>(data);

    query->_owner->onQueryResult(status,
                                 hostent,
                                 query->_hostname,
                                 query->_callback);
    delete query;
}

int AresResolver::ares_sock_create_callback(int sockfd, int type, void* data)
{
    LOG_TRACE << "sockfd=" << sockfd << " type=" << getSocketType(type);
    static_cast<AresResolver*>(data)->onSockCreate(sockfd, type);
    return 0;
}

void AresResolver::ares_sock_state_callback(void* data,
                                            int sockfd,
                                            int read,
                                            int write)
{
    LOG_TRACE << "sockfd=" << sockfd << " read=" << read << " write=" << write;
    static_cast<AresResolver*>(data)->onSockStateChange(sockfd, read, write);
}
