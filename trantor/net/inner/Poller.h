#pragma once
#include "NonCopyable.h"
#include "EventLoop.h"

#include <memory>
#include <map>

namespace trantor
{
class Channel;

class Poller : NonCopyable
{
  public:
	explicit Poller(EventLoop *loop) : ownerLoop_(loop){};
	virtual ~Poller(){}
	void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }
	virtual void poll(int timeoutMs, ChannelList *activeChannels) = 0;
	virtual void updateChannel(Channel *channel) = 0;
	virtual void removeChannel(Channel *channel) = 0;
	virtual void resetAfterFork(){}
	static Poller *newPoller(EventLoop *loop);

  private:
	EventLoop *ownerLoop_;
};
} // namespace trantor
