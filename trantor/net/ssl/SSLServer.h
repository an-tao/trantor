// Copyright 2018, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An
#pragma once

#include <trantor/net/callbacks.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/TcpServer.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/ssl/SSLConnection.h>
#include <unordered_map>
namespace trantor
{
    class SSLServer:public NonCopyable
    {
    public:
        SSLServer(EventLoop *loop,const InetAddress &address,const std::string &name);
        ~SSLServer();
        void start();
        void setIoLoopNum(size_t num);
        void setRecvMessageCallback(const SSLRecvMessageCallback &cb){_recvMessageCallback=cb;}
        void setConnectionCallback(const SSLConnectionCallback &cb){_connectionCallback=cb;}
        void setWriteCompleteCallback(const SSLWriteCompleteCallback &cb){_writeCompleteCallback=cb;}

        void setRecvMessageCallback(SSLRecvMessageCallback &&cb){_recvMessageCallback=std::move(cb);}
        void setConnectionCallback(SSLConnectionCallback &&cb){_connectionCallback=std::move(cb);}
        void setWriteCompleteCallback(SSLWriteCompleteCallback &&cb){_writeCompleteCallback=std::move(cb);}

        const std::string & name() const{return _tcpServer->name();}
        const std::string ipPort() const{return _tcpServer->ipPort();}

        EventLoop * getLoop()const {return _loop;}
    private:
        EventLoop *_loop;
        std::unique_ptr<TcpServer> _tcpServer;
        SSLRecvMessageCallback _recvMessageCallback;
        SSLConnectionCallback _connectionCallback;
        SSLWriteCompleteCallback _writeCompleteCallback;
        std::string _serverName;
        void onTcpConnection(const TcpConnectionPtr&);
        void onTcpRecvMessage(const TcpConnectionPtr&,
                              MsgBuffer*);
        void onTcpWriteComplete(const TcpConnectionPtr&);
        std::unordered_map<TcpConnectionPtr,SSLConnectionPtr> _connMap;
    };
}