#include <trantor/net/Channel.h>
#include <trantor/net/EventLoop.h>
#include <poll.h>
#include <iostream>
namespace trantor
{
	const int Channel::kNoneEvent=0;
	const int Channel::kReadEvent=POLLIN|POLLPRI;
	const int Channel::kWriteEvent=POLLOUT;
	Channel::Channel(EventLoop * loop,int fd)
			:loop_(loop),
			 fd_(fd),
			 events_(0),
			 revents_(0),
			 index_(-1)
	{

	}
	void Channel::remove()
	{
		assert(events_==kNoneEvent);
		addedToLoop_ = false;
		loop_->removeChannel(this);
	}

	void Channel::update()
	{
		loop_->updateChannel(this);
	}
	void Channel::handleEvent()
	{
		if(revents_ & POLLNVAL)
		{

		}
		if(revents_ & (POLLNVAL|POLLERR))
		{
			if(errorCallback_) errorCallback_();
		}
		if(revents_ & (POLLIN|POLLPRI|POLLRDHUP))
		{
			if(readCallback_) readCallback_();
		}
		if(revents_ & POLLOUT)
		{
			if(writeCallback_) writeCallback_();
		}
	}
}
