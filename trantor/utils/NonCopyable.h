#pragma once
namespace trantor
{
class NonCopyable
{

  protected:
    NonCopyable() {}

    ~NonCopyable() {}

  private: // emphasize the following members are private
    NonCopyable(const NonCopyable &) = delete;
    //some uncopyable classes maybe support move constructor....
    //NonCopyable( NonCopyable && ) = delete;
    const NonCopyable &operator=(const NonCopyable &);
    //const NonCopyable& operator=(NonCopyable&&);
};
} // namespace trantor
