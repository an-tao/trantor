#include <trantor/net/inner/BufferNode.h>
namespace trantor
{
class MemBufferNode : public BufferNode
{
  public:
    explicit MemBufferNode(trantor::MsgBuffer &&buffer)
        : buffer_(std::move(buffer))
    {
    }
    MemBufferNode(const char *data, size_t len) : buffer_(len)
    {
        buffer_.append(data, len);
    }

    void getData(const char *&data, size_t &len) override
    {
        data = buffer_.peek();
        len = buffer_.readableBytes();
    }
    void retrieve(size_t len) override
    {
        buffer_.retrieve(len);
    }
    long long remainingBytes() const override
    {
        if (isDone_)
            return 0;
        return static_cast<long long>(buffer_.readableBytes());
    }
    void append(const char *data, size_t len) override
    {
        buffer_.append(data, len);
    }

  private:
    trantor::MsgBuffer buffer_;
};
BufferNodePtr BufferNode::newMemBufferNode(trantor::MsgBuffer buffer)
{
    return std::make_shared<MemBufferNode>(std::move(buffer));
}
BufferNodePtr BufferNode::newMemBufferNode(const char *data, size_t len)
{
    return std::make_shared<MemBufferNode>(data, len);
}
}  // namespace trantor
