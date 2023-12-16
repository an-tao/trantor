#include <trantor/net/inner/BufferNode.h>
namespace trantor
{
static const size_t kMaxSendFileBufferSize = 16 * 1024;
class FileBufferNode : public BufferNode
{
  public:
    FileBufferNode(FILE *fp, long long offset, size_t length)
        : sendFp_(fp), fileBytesToSend_(length)
    {
        assert(fp);
        _fseeki64(sendFp_, offset, SEEK_SET);
    }

    bool isFile() const override
    {
        return true;
    }

    void getData(const char *&data, size_t &len) override
    {
        if (msgBuffer_.readableBytes() == 0)
        {
            msgBuffer_.ensureWritableBytes(kMaxSendFileBufferSize <
                                                   fileBytesToSend_
                                               ? kMaxSendFileBufferSize
                                               : fileBytesToSend_);
            auto n = fread(msgBuffer_.beginWrite(),
                           1,
                           msgBuffer_.writableBytes(),
                           sendFp_);
            if (n > 0)
            {
                msgBuffer_.hasWritten(n);
            }
            else if (n == 0)
            {
                LOG_TRACE << "Read the end of file.";
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
    ~FileBufferNode() override
    {
        if (sendFp_)
        {
            fclose(sendFp_);
        }
    }
    int getFd() const override
    {
        LOG_ERROR << "getFd() is not supported on Windows";
        return 0;
    }

  private:
    FILE *sendFp_{nullptr};
    long long offset_{0};

    size_t fileBytesToSend_{0};

    MsgBuffer msgBuffer_;
};
BufferNodePtr BufferNode::newFileBufferNode(FILE *fp,
                                            long long offset,
                                            size_t length)
{
    return std::make_shared<FileBufferNode>(fp, offset, length);
}
}  // namespace trantor