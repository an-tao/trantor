#include <trantor/net/inner/BufferNode.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>

namespace trantor
{
static const size_t kMaxSendFileBufferSize = 16 * 1024;
class FileBufferNode : public BufferNode
{
  public:
    FileBufferNode(const char *fileName, off_t offset, size_t length)
    {
        sendFd_ = open(fileName, O_RDONLY);

        if (sendFd_ < 0)
        {
            LOG_SYSERR << fileName << " open error";
            isDone_ = true;
            return;
        }
        struct stat filestat;
        if (stat(fileName, &filestat) < 0)
        {
            LOG_SYSERR << fileName << " stat error";
            close(sendFd_);
            sendFd_ = -1;
            isDone_ = true;
            return;
        }
        if (length == 0)
        {
            if (offset >= filestat.st_size)
            {
                LOG_ERROR << "The file size is " << filestat.st_size
                          << " bytes, but the offset is " << offset
                          << " bytes and the length is " << length << " bytes";
                close(sendFd_);
                sendFd_ = -1;
                isDone_ = true;
                return;
            }
            fileBytesToSend_ = filestat.st_size - offset;
        }
        else
        {
            if (length + offset > filestat.st_size)
            {
                LOG_ERROR << "The file size is " << filestat.st_size
                          << " bytes, but the offset is " << offset
                          << " bytes and the length is " << length << " bytes";
                close(sendFd_);
                sendFd_ = -1;
                isDone_ = true;
                return;
            }
            fileBytesToSend_ = length;
        }
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
        if (msgBuffer_.readableBytes() == 0 && fileBytesToSend_ > 0 &&
            sendFd_ >= 0)
        {
            msgBuffer_.ensureWritableBytes(
                (std::min)(kMaxSendFileBufferSize, fileBytesToSend_));
            auto n = read(sendFd_,
                          msgBuffer_.beginWrite(),
                          msgBuffer_.writableBytes());
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
        if (sendFd_ >= 0)
        {
            close(sendFd_);
        }
    }
    bool available() const override
    {
        return sendFd_ >= 0;
    }

  private:
    int sendFd_{-1};
    size_t fileBytesToSend_{0};
    MsgBuffer msgBuffer_;
};

BufferNodePtr BufferNode::newFileBufferNode(int fd, off_t offset, size_t length)
{
    return std::make_shared<FileBufferNode>(fd, offset, length);
}
}  // namespace trantor