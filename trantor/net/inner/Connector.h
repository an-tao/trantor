#include <trantor/net/EventLoop.h>
#include <trantor/net/InetAddress.h>
#include <atomic>
#include <memory>
namespace trantor
{
    class Connector:public NonCopyable,public std::enable_shared_from_this<Connector>
    {
    public:
        typedef std::function<void (int sockfd)> NewConnectionCallback;
        typedef std::function<void()> ConnectionErrorCallback;
        Connector(EventLoop *loop,const InetAddress &addr,bool retry=true);
        Connector(EventLoop *loop,InetAddress &&addr,bool retry=true);
        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_=cb;
        }
        void setNewConnectionCallback(NewConnectionCallback &&cb)
        {
            newConnectionCallback_=std::move(cb);
        }
        void setErrorCallback(const ConnectionErrorCallback& cb)
        {
            _errorCallback=cb;
        }
        void setErrorCallback(ConnectionErrorCallback &&cb)
        {
            _errorCallback=std::move(cb);
        }
        const InetAddress& serverAddress() const { return serverAddr_; }
        void start();
        void restart();
        void stop();
    private:
        NewConnectionCallback newConnectionCallback_;
        ConnectionErrorCallback _errorCallback;
        enum States { kDisconnected, kConnecting, kConnected };
        static const int kMaxRetryDelayMs = 30*1000;
        static const int kInitRetryDelayMs = 500;
        std::shared_ptr<Channel> channelPtr_;
        EventLoop *loop_;
        InetAddress serverAddr_;

        std::atomic_bool connect_;
        std::atomic<States> state_;

        int retryInterval_;
        int maxRetryInterval_;

        bool _retry;

        void startInLoop();
        void connect();
        void connecting(int sockfd);
        int removeAndResetChannel();
        void handleWrite();
        void handleError();
        void retry(int sockfd);
    };
}