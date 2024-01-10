#pragma once

#include <memory>
#include <string>
#include <functional>

#include <trantor/net/TcpConnection.h>
#include <trantor/utils/Logger.h>

namespace trantor
{

enum class PipelineMode
{
    UnpackByDelimiter,
    UnpackByFixedLength,
};

struct PipelineSetting
{
    PipelineMode mode_{PipelineMode::UnpackByDelimiter};
    std::string delimiter_{"\n"};
    std::size_t packageMaxLength_{1024};
    std::size_t fixedLength_;
    std::function<void(std::string)> noticeCallback_{nullptr};
};

class Pipeline
{
  public:
    Pipeline(const PipelineSetting &PipelineSetting)
        : PipelineSetting_(PipelineSetting)
    {
    }

    void unpack(const TcpConnectionPtr &connectionPtr,
                MsgBuffer *buffer,
                const std::function<void(const std::shared_ptr<TcpConnection> &,
                                         trantor::MsgBuffer *)> &cb)
    {
        switch (PipelineSetting_.mode_)
        {
            case PipelineMode::UnpackByDelimiter:
            {
                while (true)
                {
                    const char *ret =
                        std::search(buffer->peek(),
                                    (const char *)buffer->beginWrite(),
                                    PipelineSetting_.delimiter_.begin(),
                                    PipelineSetting_.delimiter_.end());
                    if (ret == buffer->beginWrite())
                    {
                        if (buffer->readableBytes() >
                            PipelineSetting_.packageMaxLength_)
                        {
                            LOG_ERROR << "> PipelineSetting package max length";
                            if (PipelineSetting_.noticeCallback_)
                                PipelineSetting_.noticeCallback_(
                                    std::string(buffer->peek(),
                                                buffer->readableBytes()));
                            buffer->retrieveAll();
                            connectionPtr->shutdown();
                            return;
                        }
                        break;
                    }
                    else
                    {
                        MsgBuffer buf;
                        buf.append(buffer->peek(), ret - buffer->peek());
                        buffer->retrieveUntil(
                            ret + PipelineSetting_.delimiter_.size());
                        cb(connectionPtr, &buf);
                    }
                }
                break;
            }
            case PipelineMode::UnpackByFixedLength:
            {
                while (buffer->readableBytes() >= PipelineSetting_.fixedLength_)
                {
                    MsgBuffer buf;
                    buf.append(buffer->peek(), PipelineSetting_.fixedLength_);
                    buffer->retrieveUntil(buffer->peek() +
                                          PipelineSetting_.fixedLength_);
                    cb(connectionPtr, &buf);
                }
                break;
            }
            default:
            {
                LOG_ERROR << "invalid unpack setting mode";
                break;
            }
        }
    }

  private:
    PipelineSetting PipelineSetting_;
};

}  // namespace trantor
