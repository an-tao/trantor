#include <trantor/net/Acceptor.h>
using namespace trantor;
Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr) :
        sock_(Socket::createNonblockingSocketOrDie(addr.getSockAddr()->sa_family)),
        addr_(addr),
        loop_(loop),
        acceptChannel_(loop,sock_.fd())
{
    sock_.setReuseAddr(true);
    sock_.setReusePort(true);

    sock_.bindAddress(addr_);

    acceptChannel_.setReadCallback(std::bind(&Acceptor::readCallback,this));
}
Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}
void Acceptor::listen() {
    loop_->assertInLoopThread();
    sock_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::readCallback() {
    InetAddress peer;
    int newsock=sock_.accept(&peer);
    if(newsock>=0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(newsock,peer);
        } else
        {
            close(newsock);
        }
    } else
    {
        LOG_SYSERR<<"Accpetor::readCallback";
    }

}