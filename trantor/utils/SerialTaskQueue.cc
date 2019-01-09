#include <trantor/utils/SerialTaskQueue.h>
#include <trantor/utils/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
namespace trantor
{
SerialTaskQueue::SerialTaskQueue(const std::string &name)
    : _queueName(name),
      _stop(false),
      _thread(std::bind(&SerialTaskQueue::queueFunc, this)),
      _isRuningTask(false)
{
    if (name.empty())
    {
        _queueName = "SerailTaskQueue";
    }
    //LOG_TRACE<<"construct SerialTaskQueue('"<<_queueName<<"')";
}
void SerialTaskQueue::stop()
{
    _stop = true;
    _taskCond.notify_all();
    _thread.join();
}
SerialTaskQueue::~SerialTaskQueue()
{
    if (!_stop)
        stop();
    LOG_TRACE << "destruct SerialTaskQueue('" << _queueName << "')";
}
void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task)
{
    //LOG_TRACE<<"copy task into queue";
    std::lock_guard<std::mutex> lock(_taskMutex);
    _taskQueue.push(task);
    _taskCond.notify_one();
}
void SerialTaskQueue::runTaskInQueue(std::function<void()> &&task)
{
    //LOG_TRACE<<"move task into queue";
    std::lock_guard<std::mutex> lock(_taskMutex);
    _taskQueue.push(std::move(task));
    _taskCond.notify_one();
}

void SerialTaskQueue::queueFunc()
{
#ifdef __linux__
    ::prctl(PR_SET_NAME, _queueName.c_str());
#endif
    while (!_stop)
    {
        std::function<void()> r;
        {
            std::unique_lock<std::mutex> lock(_taskMutex);
            while (!_stop && _taskQueue.size() == 0)
            {
                _taskCond.wait(lock);
            }
            if (_taskQueue.size() > 0)
            {
                //LOG_TRACE<<"got a new task!";
                r = std::move(_taskQueue.front());
                _taskQueue.pop();
            }
            else
                continue;
        }
        _isRuningTask = true;
        r();
        _isRuningTask = false;
    }
}
void SerialTaskQueue::waitAllTasksFinished()
{
    syncTaskInQueue([]() {

    });
}
size_t SerialTaskQueue::getTaskCount()
{
    std::lock_guard<std::mutex> guard(_taskMutex);
    return _taskQueue.size();
}
}; // namespace trantor
