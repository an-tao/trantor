#pragma once
#include <trantor/utils/NonCopyable.h>
#include <functional>
#include <assert.h>
namespace trantor
{
	class EventLoop;
	class Channel:NonCopyable
	{
	public:
		typedef std::function<void()> EventCallback;
		Channel(EventLoop *loop,int fd);
		void handleEvent();
		void setReadCallback(const EventCallback& cb)
		{
			readCallback_=cb;
		};
		void setWriteCallback(const EventCallback& cb)
		{
			writeCallback_=cb;
		};
		int fd() const {return fd_;};
		int events() const {return events_;};
		int setRevents(int revt) { revents_=revt;};
		bool isNoneEvent() const {return events_ == kNoneEvent;};
		int index(){return index_;};
		void setIndex(int index){ index_=index;};
        void disableAll() { events_ = kNoneEvent; update(); }
        void remove();

		EventLoop *ownerLoop(){return loop_;};

		void enableReading(){events_|=kReadEvent;update();};
	private:
		void update();
		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;

		EventLoop *loop_;
		const int fd_;
		int events_;
		int revents_;
		int index_;
        bool addedToLoop_=false;
		EventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback errorCallback_;

	};
};