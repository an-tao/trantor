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
Acceptor::Acceptor(EventLoop *loop,
                   const InetAddress &addr,
                   bool reUseAddr,
                   bool reUsePort)
    : sock_(
          Socket::createNonblockingSocketOrDie(addr.getSockAddr()->sa_family)),
      addr_(addr),
      loop_(loop),
      acceptChannel_(loop, sock_.fd()),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    sock_.setReuseAddr(reUseAddr);
    sock_.setReusePort(reUsePort);
    sock_.bindAddress(addr_);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::readCallback, this));
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}
void Acceptor::listen()
{
    loop_->assertInLoopThread();
    sock_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::readCallback()
{
    InetAddress peer;
    int newsock = sock_.accept(&peer);
    if (newsock >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(newsock, peer);
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
            ::close(idleFd_);
            idleFd_ = sock_.accept(&peer);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
