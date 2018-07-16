#pragma once
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/EventLoop.h>

#include <memory>
#include <map>
typedef std::vector<struct epoll_event> EventList;

namespace trantor
{
	class Channel;


	class Poller :NonCopyable
	{
	public:
		explicit Poller(EventLoop *loop);
		~Poller();
		void assertInLoopThread(){ownerLoop_->assertInLoopThread();}
		void poll(int timeoutMs, ChannelList* activeChannels);
		void updateChannel(Channel* channel);
		void removeChannel(Channel* channel);
	private:

		EventLoop* ownerLoop_;
		static const int kInitEventListSize = 16;
		int epollfd_;
		EventList events_;
		void update(int operation, Channel* channel);
		typedef std::map<int, Channel*> ChannelMap;
		ChannelMap channels_;
		void fillActiveChannels(int numEvents,
								ChannelList* activeChannels) const;

	};
}
