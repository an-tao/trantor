#pragma once

#include <trantor/utils/config.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/utils/MsgBuffer.h>
#include <trantor/net/callbacks.h>
#include <memory>
#include <functional>

#ifdef USE_STD_ANY

  #include <any>
  using std::any;
  using std::any_cast;

#else 
  #ifdef USE_BOOST

    #include <boost/any.hpp>
    using boost::any;
    using boost::any_cast;

  #else
    #error,must use c++17 or boost
  #endif
#endif

namespace trantor
{
    class TcpConnection
    {
    public:
        TcpConnection()= default;
        virtual ~TcpConnection(){};
        virtual void send(const char *msg,uint64_t len)=0;
        virtual void send(const std::string &msg)=0;

        virtual const InetAddress& lobalAddr() const=0;
        virtual const InetAddress& peerAddr() const=0;

        virtual bool connected() const =0;
        virtual bool disconnected() const =0;

        virtual MsgBuffer* getSendBuffer() =0;
        virtual MsgBuffer* getRecvBuffer() =0;
        //set callbacks
        virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t markLen)=0;

        virtual void setTcpNoDelay(bool on)=0;
        virtual void shutdown()=0;
        virtual void forceClose()=0;
        virtual EventLoop* getLoop()=0;

#ifdef NO_ANY
        virtual void setContext(void *p)const=0;
        virtual void* getContext()const=0;
#else
        virtual void setContext(const any& context)=0;

        virtual const any& getContext() const=0;

        virtual any* getMutableContext()=0;
#endif
    };
}
