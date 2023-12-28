#include <trantor/net/inner/BufferNode.h>

namespace trantor
{
class AsyncBufferNode : public BufferNode
{
  public:
    AsyncBufferNode() = default;
    ~AsyncBufferNode() override = default;
    bool isAsync() const override
    {
        return true;
    }
    bool isStream() const override
    {
        return true;
    }
    size_t remainingBytes() const override
    {
        if (msgBufferPtr_)
            return msgBufferPtr_->readableBytes();
        return 0;
    }
    bool available() const override
    {
        return !isDone_;
    }
    void getData(const char *&data, size_t &len) override
    {
        if (msgBufferPtr_)
        {
            data = msgBufferPtr_->peek();
            len = msgBufferPtr_->readableBytes();
        }
        else
        {
            data = nullptr;
            len = 0;
        }
    }
    void retrieve(size_t len) override
    {
        assert(msgBufferPtr_);
        if (msgBufferPtr_)
        {
            msgBufferPtr_->retrieve(len);
        }
    }
    void append(const char *data, size_t len) override
    {
        if (!msgBufferPtr_)
        {
            msgBufferPtr_ = std::make_unique<MsgBuffer>(len);
        }
        msgBufferPtr_->append(data, len);
    }

  private:
    std::unique_ptr<MsgBuffer> msgBufferPtr_;
};
BufferNodePtr BufferNode::newAsyncStreamBufferNode()
{
    return std::make_shared<AsyncBufferNode>();
}
}  // namespace trantor
