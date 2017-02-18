#pragma once
namespace trantor
{
    class NonCopyable
    {

    protected:

        NonCopyable() {}

        ~NonCopyable() {}

    private:  // emphasize the following members are private

        NonCopyable( const NonCopyable& ) = delete;
        NonCopyable( NonCopyable && ) = delete;
        const NonCopyable& operator=( const NonCopyable& );
        const NonCopyable& operator=(NonCopyable&&);

    };
}

