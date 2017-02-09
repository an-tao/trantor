#pragma once

#include <trantor/net/EventLoop.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/Socket.h>
#include <trantor/net/InetAddress.h>
#include <trantor/net/Channel.h>
#include <functional>

namespace trantor
{
    typedef std::function<void (int fd,const InetAddress &)> NewConnectionCallback;
    class Acceptor:NonCopyable
    {
    public:

        Acceptor(EventLoop* loop, const InetAddress& addr);
        ~Acceptor();
        const InetAddress & addr() const {return addr_;}
        void setNewConnectionCallback(const NewConnectionCallback &cb){
            newConnectionCallback_=cb;
        };
        void listen();
    protected:
        Socket sock_;
        InetAddress addr_;
        EventLoop *loop_;
        NewConnectionCallback newConnectionCallback_;
        Channel acceptChannel_;
        void readCallback();
    };
}