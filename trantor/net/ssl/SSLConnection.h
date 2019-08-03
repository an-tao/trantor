/**
 *
 *  SSLConnection.h
 *  An Tao
 *
 *  Public header file in trantor lib.
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the License file.
 *
 *
 */

#pragma once
#include "../inner/TcpConnectionImpl.h"
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/TcpConnection.h>
#include <memory>
namespace trantor
{
enum class SSLStatus
{
    Handshaking,
    Connecting,
    Connected,
    DisConnecting,
    DisConnected
};
void initOpenSSL();
class SSLContext;
class SSLConn;

std::shared_ptr<SSLContext> newSSLContext();
void initServerSSLContext(const std::shared_ptr<SSLContext> &ctx,
                          const std::string &certPath,
                          const std::string &keyPath);

class SSLConnection : public TcpConnectionImpl
{
  public:
    SSLConnection(EventLoop *loop,
                  int socketfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr,
                  const std::shared_ptr<SSLContext> &ctxPtr,
                  bool isServer = true);
    virtual ~SSLConnection()
    {
    }

  private:
    void doHandshaking();
    SSLStatus _status = SSLStatus::Handshaking;

  private:
    // OpenSSL
    std::shared_ptr<SSLConn> _sslPtr;
    bool _isServer;

  protected:
    virtual void readCallback() override;
    virtual void writeCallback() override;
    virtual void connectEstablished() override;
    virtual ssize_t writeInLoop(const char *buffer, size_t length) override;
    char _sendBuffer[8192];
};

typedef std::shared_ptr<SSLConnection> SSLConnectionPtr;

}  // namespace trantor
