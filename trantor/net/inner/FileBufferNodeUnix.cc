#include <trantor/net/inner/BufferNode.h>

namespace trantor
{
static const size_t kMaxSendFileBufferSize = 16 * 1024;
class FileBufferNode : public BufferNode
{
  public:
    FileBufferNode(int fd, off_t offset, size_t length)
        : sendFd_(fd), fileBytesToSend_(length)
    {
        assert(fd >= 0);
        lseek(sendFd_, offset, SEEK_SET);
    }
    bool isFile() const override
    {
        return true;
    }
    int getFd() const override
    {
        return sendFd_;
    }
    void getData(const char *&data, size_t &len) override
    {
        if (msgBuffer_.readableBytes() == 0)
        {
            msgBuffer_.ensureWritableBytes(
                std::min(kMaxSendFileBufferSize, fileBytesToSend_));
            auto n = read(sendFd_,
                          msgBuffer_.beginWrite(),
                          msgBuffer_.writableBytes());
            if (n > 0)
            {
                msgBuffer_.hasWritten(n);
            }
            else
            {
                LOG_SYSERR << "FileBufferNode::getData()";
            }
        }
        data = msgBuffer_.peek();
        len = msgBuffer_.readableBytes();
    }
    void retrieve(size_t len) override
    {
        msgBuffer_.retrieve(len);
        fileBytesToSend_ -= len;
    }
    size_t remainingBytes() const override
    {
        if (isDone_)
            return 0;
        return fileBytesToSend_;
    }
    ~FileBufferNode() override;

  private:
    int sendFd_{-1};
    ssize_t fileBytesToSend_{0};
    MsgBuffer msgBuffer_;
};

BufferNodePtr BufferNode::newFileBufferNode(int fd, off_t offset, size_t length)
{
    return std::make_shared<FileBufferNode>(fd, offset, length);
}
}  // namespace trantor