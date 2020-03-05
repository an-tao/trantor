#include "NormalResolver.h"
#include <trantor/utils/Logger.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // memset
#endif

using namespace trantor;

std::shared_ptr<Resolver> Resolver::newResolver(trantor::EventLoop *loop,
                                                size_t timeout)
{
    return std::make_shared<NormalResolver>(timeout);
}

void NormalResolver::resolve(const std::string &hostname,
                             const Callback &callback)
{
    {
        std::lock_guard<std::mutex> guard(globalMutex());
        auto iter = globalCache().find(hostname);
        if (iter != globalCache().end())
        {
            auto &cachedAddr = iter->second;
            if (timeout_ == 0 || cachedAddr.second.after(static_cast<double>(
                                     timeout_)) > trantor::Date::date())
            {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof addr);
                addr.sin_family = AF_INET;
                addr.sin_port = 0;
                addr.sin_addr = cachedAddr.first;
                InetAddress inet(addr);
                callback(inet);
                return;
            }
        }
    }

    concurrentTaskQueue().runTaskInQueue(
        [thisPtr = shared_from_this(), callback, hostname]() {
            {
                std::lock_guard<std::mutex> guard(thisPtr->globalMutex());
                auto iter = thisPtr->globalCache().find(hostname);
                if (iter != thisPtr->globalCache().end())
                {
                    auto &cachedAddr = iter->second;
                    if (thisPtr->timeout_ == 0 ||
                        cachedAddr.second.after(static_cast<double>(thisPtr->timeout_)) >
                            trantor::Date::date())
                    {
                        struct sockaddr_in addr;
                        memset(&addr, 0, sizeof addr);
                        addr.sin_family = AF_INET;
                        addr.sin_port = 0;
                        addr.sin_addr = cachedAddr.first;
                        InetAddress inet(addr);
                        callback(inet);
                        return;
                    }
                }
            }
#ifdef __linux__
            struct hostent hent;
            struct hostent *he = NULL;
            int herrno = 0;
            memset(&hent, 0, sizeof(hent));
            int ret;
            while (1)
            {
                ret = gethostbyname_r(hostname.c_str(),
                                      &hent,
                                      thisPtr->resolveBuffer_.data(),
                                      thisPtr->resolveBuffer_.size(),
                                      &he,
                                      &herrno);
                if (ret == ERANGE)
                {
                    thisPtr->resolveBuffer_.resize(
                        thisPtr->resolveBuffer_.size() * 2);
                }
                else
                {
                    break;
                }
            }
            if (ret == 0 && he != NULL)
#else
            /// Multi-threads safety
            static std::mutex mutex_;
            struct hostent *he = NULL;
            struct hostent hent;
            {
                std::lock_guard<std::mutex> guard(mutex_);
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
                assert(he->h_addrtype == AF_INET &&
                       he->h_length == sizeof(uint32_t));

                struct sockaddr_in addr;
                memset(&addr, 0, sizeof addr);
                addr.sin_family = AF_INET;
                addr.sin_port = 0;
                addr.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
                InetAddress inet(addr);
                callback(inet);
                {
                    std::lock_guard<std::mutex> guard(thisPtr->globalMutex());
                    auto &addrItem = thisPtr->globalCache()[hostname];
                    addrItem.first = addr.sin_addr;
                    addrItem.second = trantor::Date::date();
                }
                return;
            }
            else
            {
                LOG_SYSERR << "InetAddress::resolve";
                callback(InetAddress{});
                return;
            }
        });
}
