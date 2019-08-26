// Copyright 2016, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An

#pragma once
#include <trantor/net/Resolver.h>
#include <trantor/utils/NonCopyable.h>
#include <map>
#include <memory>

extern "C"
{
    struct hostent;
    struct ares_channeldata;
    typedef struct ares_channeldata* ares_channel;
}
namespace trantor
{
class AresResolver : public Resolver,
                     public NonCopyable,
                     public std::enable_shared_from_this<AresResolver>
{
  public:
    AresResolver(trantor::EventLoop* loop, size_t timeout);
    ~AresResolver();

    virtual void resolve(const std::string& hostname,
                         const Callback& cb) override
    {
        if (_loop->isInLoopThread())
        {
            resolveInLoop(hostname, cb);
        }
        else
        {
            _loop->queueInLoop([thisPtr = shared_from_this(), hostname, cb]() {
                thisPtr->resolveInLoop(hostname, cb);
            });
        }
    }

  private:
    struct QueryData
    {
        AresResolver* _owner;
        Callback _callback;
        std::string _hostname;
        QueryData(AresResolver* o,
                  const Callback& cb,
                  const std::string& hostname)
            : _owner(o), _callback(cb), _hostname(hostname)
        {
        }
    };
    void resolveInLoop(const std::string& hostname, const Callback& cb);

    trantor::EventLoop* _loop;
    ares_channel _ctx;
    bool _timerActive;
    typedef std::map<int, std::unique_ptr<trantor::Channel>> ChannelList;
    ChannelList _channels;
    static std::unordered_map<std::string,
                              std::pair<struct in_addr, trantor::Date>>&
    globalCache()
    {
        static std::unordered_map<std::string,
                                  std::pair<struct in_addr, trantor::Date>>
            _dnsCache;
        return _dnsCache;
    }
    static std::mutex& globalMutex()
    {
        static std::mutex _mutex;
        return _mutex;
    }

    const size_t _timeout = 60;

    void onRead(int sockfd);
    void onTimer();
    void onQueryResult(int status,
                       struct hostent* result,
                       const std::string& hostname,
                       const Callback& callback);
    void onSockCreate(int sockfd, int type);
    void onSockStateChange(int sockfd, bool read, bool write);

    static void ares_host_callback(void* data,
                                   int status,
                                   int timeouts,
                                   struct hostent* hostent);
    static int ares_sock_create_callback(int sockfd, int type, void* data);
    static void ares_sock_state_callback(void* data,
                                         int sockfd,
                                         int read,
                                         int write);
};
}  // namespace trantor