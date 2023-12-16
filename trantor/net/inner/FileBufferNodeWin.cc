#include <trantor/net/inner/BufferNode.h>
namespace trantor
{
static const size_t kMaxSendFileBufferSize = 16 * 1024;
class FileBufferNode : public BufferNode
{
  public:
    FileBufferNode(const wchar_t *fileName, long long offset, size_t length)
    {
#ifndef _MSC_VER
        sendFp_ = _wfopen(fileName, L"rb");
#else   // _MSC_VER
        if (_wfopen_s(&sendFp_, fileName, L"rb") != 0)
            sendFp_ = nullptr;
#endif  // _MSC_VER
        if (sendFp_ == nullptr)
        {
            LOG_SYSERR << fileName << " open error";
            isDone_ = true;
            return;
        }
        struct _stati64 filestat;
        if (_wstati64(fileName, &filestat) < 0)
        {
            LOG_SYSERR << fileName << " stat error";
            fclose(sendFp_);
            sendFp_ = nullptr;
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
                fclose(sendFp_);
                sendFp_ = nullptr;
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
                fclose(sendFp_);
                sendFp_ = nullptr;
                isDone_ = true;
                return;
            }
            fileBytesToSend_ = length;
        }
        _fseeki64(sendFp_, offset, SEEK_SET);
    }

    bool isFile() const override
    {
        return true;
    }

    void getData(const char *&data, size_t &len) override
    {
        if (msgBuffer_.readableBytes() == 0 && fileBytesToSend_ > 0 && sendFp_)
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
    bool available() const override
    {
        return sendFp_ != nullptr;
    }

  private:
    FILE *sendFp_{nullptr};
    size_t fileBytesToSend_{0};
    MsgBuffer msgBuffer_;
};
BufferNodePtr BufferNode::newFileBufferNode(const wchar_t *fileName,
                                            long long offset,
                                            size_t length)
{
    return std::make_shared<FileBufferNode>(fileName, offset, length);
}
}  // namespace trantor