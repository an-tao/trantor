#include <trantor/net/EventLoop.h>
#include <trantor/net/EventLoopThread.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace trantor;

// Regression test for a data race between EventLoop::quit() called from a
// foreign thread and the destruction of the EventLoop. EventLoopThread
// destroys its loop in the loop thread when the thread exits, which can
// happen as soon as the loop thread observes the quit flag, i.e. before
// quit() has finished waking the loop up (write to the wakeup fd).
//
// The test is primarily meant to be run under ThreadSanitizer, which reports
// the race deterministically. Without it, it still checks that the shutdown
// sequence neither crashes nor hangs.
TEST(EventLoopQuit, QuitFromForeignThreadWhileLoopThreadExits)
{
    constexpr int kIterations = 500;
    for (int i = 0; i < kIterations; ++i)
    {
        EventLoopThread loopThread("QuitRaceLoop");
        loopThread.run();
        EventLoop *loop = loopThread.getLoop();
        ASSERT_NE(loop, nullptr);

        std::promise<void> inLoop;
        std::atomic<bool> aboutToQuit{false};
        // Keep the loop thread busy outside of poll() until quit() is about
        // to be called, so that it sees the quit flag right away and leaves
        // loop() without needing to be woken up.
        loop->queueInLoop([&inLoop, &aboutToQuit]() {
            inLoop.set_value();
            while (!aboutToQuit.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        });
        inLoop.get_future().wait();

        aboutToQuit.store(true, std::memory_order_release);
        loop->quit();
        // Joining the thread guarantees the loop has been destroyed.
        loopThread.wait();
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
