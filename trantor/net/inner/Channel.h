#pragma once

#include <trantor/utils/Logger.h>
#include <trantor/utils/NonCopyable.h>
#include <functional>
#include <assert.h>
#include <memory>
namespace trantor
{
class EventLoop;
class Channel : NonCopyable
{
  public:
	typedef std::function<void()> EventCallback;
	Channel(EventLoop *loop, int fd);
	void handleEvent();
	void setReadCallback(const EventCallback &cb)
	{
		readCallback_ = cb;
	};
	void setWriteCallback(const EventCallback &cb)
	{
		writeCallback_ = cb;
	};
	void setCloseCallback(const EventCallback &cb)
	{
		closeCallback_ = cb;
	}
	void setErrorCallback(const EventCallback &cb)
	{
		errorCallback_ = cb;
	}

	void setReadCallback(EventCallback &&cb)
	{
		readCallback_ = std::move(cb);
	}
	void setWriteCallback(EventCallback &&cb)
	{
		writeCallback_ = std::move(cb);
	}
	void setCloseCallback(EventCallback &&cb)
	{
		closeCallback_ = std::move(cb);
	}
	void setErrorCallback(EventCallback &&cb)
	{
		errorCallback_ = std::move(cb);
	}

	int fd() const { return fd_; };
	int events() const { return events_; };
	int setRevents(int revt)
	{
		//LOG_TRACE<<"revents="<<revt;
		revents_ = revt;
		return revt;
	};
	bool isNoneEvent() const { return events_ == kNoneEvent; };
	int index() { return index_; };
	void setIndex(int index) { index_ = index; };
	void disableAll()
	{
		events_ = kNoneEvent;
		update();
	}
	void remove();
	void tie(const std::shared_ptr<void> &obj)
	{
		tie_ = obj;
		tied_ = true;
	}
	EventLoop *ownerLoop() { return loop_; };

	void enableReading()
	{
		events_ |= kReadEvent;
		update();
	}
	void disableReading()
	{
		events_ &= ~kReadEvent;
		update();
	}
	void enableWriting()
	{
		events_ |= kWriteEvent;
		update();
	}
	void disableWriting()
	{
		events_ &= ~kWriteEvent;
		update();
	}

	bool isWriting() const { return events_ & kWriteEvent; }
	bool isReading() const { return events_ & kReadEvent; }

  private:
	void update();
	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;
	void handleEventSafely();
	EventLoop *loop_;
	const int fd_;
	int events_;
	int revents_;
	int index_;
	bool addedToLoop_ = false;
	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback errorCallback_;
	EventCallback closeCallback_;
	std::weak_ptr<void> tie_;
	bool tied_;
};
}; // namespace trantor
