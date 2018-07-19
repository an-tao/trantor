// Copyright 2018, Tao An.  All rights reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Tao An
#pragma once

#include <trantor/utils/NonCopyable.h>
#include <trantor/net/TcpConnection.h>
#include <memory>
namespace trantor
{
    enum class SSLStatus{
        Init,
        Connecting,
        Connected,
        DisConnecting,
        DisConnected
    };
    class SSLConnection:public NonCopyable{
    public:
        SSLConnection(const TcpConnectionPtr &);
        ~SSLConnection(){}
        void send(const char *msg,uint64_t len);
        void send(const std::string &msg);

        const InetAddress& lobalAddr() const {return _tcpConn->lobalAddr();}
        const InetAddress& peerAddr() const {return _tcpConn->peerAddr();}
    private:
        TcpConnectionPtr _tcpConn;
        SSLStatus _status=SSLStatus::Init;
    };
    typedef std::shared_ptr<SSLConnection> SSLConnectionPtr;

}