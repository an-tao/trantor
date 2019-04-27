/**
 *
 *  Acceptor.cc
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

#include "Acceptor.h"
using namespace trantor;
Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr, bool reUseAddr, bool reUsePort)
    : _sock(Socket::createNonblockingSocketOrDie(addr.getSockAddr()->sa_family)),
      _addr(addr),
      _loop(loop),
      _acceptChannel(loop, _sock.fd()),
      _idleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    _sock.setReuseAddr(reUseAddr);
    _sock.setReusePort(reUsePort);

    _sock.bindAddress(_addr);

    _acceptChannel.setReadCallback(std::bind(&Acceptor::readCallback, this));
}
Acceptor::~Acceptor()
{
    _acceptChannel.disableAll();
    _acceptChannel.remove();
    ::close(_idleFd);
}
void Acceptor::listen()
{
    _loop->assertInLoopThread();
    _sock.listen();
    _acceptChannel.enableReading();
}

void Acceptor::readCallback()
{
    InetAddress peer;
    int newsock = _sock.accept(&peer);
    if (newsock >= 0)
    {
        if (_newConnectionCallback)
        {
            _newConnectionCallback(newsock, peer);
        }
        else
        {
            close(newsock);
        }
    }
    else
    {
        LOG_SYSERR << "Accpetor::readCallback";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        /// errno is thread safe
        if (errno == EMFILE)
        {
            ::close(_idleFd);
            _idleFd = _sock.accept(&peer);
            ::close(_idleFd);
            _idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
