/**
 *
 *  @file Utilities.cc
 *  @author An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#include "Utilities.h"
#ifdef _WIN32
#include <Windows.h>
#include <ntsecapi.h>
#include <algorithm>
#else  // _WIN32
#include <unistd.h>
#include <chrono>
#include <string.h>
#if __cplusplus < 201103L || __cplusplus >= 201703L
#include <stdlib.h>
#include <locale.h>
#else  // __cplusplus
#include <locale>
#include <codecvt>
#endif  // __cplusplus
#endif  // _WIN32

#if defined(USE_OPENSSL)
#include <openssl/rand.h>
#elif defined(USE_BOTAN)
#include <botan/auto_rng.h>
#else
#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/sha256.h"
#include "crypto/sha3.h"
#include <fstream>
#endif

#include <cassert>

namespace trantor
{
namespace utils
{
std::string toUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return {};

    std::string strTo;
#ifdef _WIN32
    int nSizeNeeded = ::WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    strTo.resize(nSizeNeeded, 0);
    ::WideCharToMultiByte(CP_UTF8,
                          0,
                          &wstr[0],
                          (int)wstr.size(),
                          &strTo[0],
                          nSizeNeeded,
                          NULL,
                          NULL);
#else  // _WIN32
#if __cplusplus < 201103L || __cplusplus >= 201703L
    // Note: Introduced in c++11 and deprecated with c++17.
    // Revert to C99 code since there no replacement yet
    strTo.resize(3 * wstr.length(), 0);
    locale_t utf8 = newlocale(LC_ALL_MASK, "C.UTF-8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.utf-8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.UTF8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.utf8", NULL);
    auto nLen = wcstombs_l(&strTo[0], wstr.c_str(), strTo.length(), utf8);
    strTo.resize(nLen);
    freelocale(utf8);
#else   // c++11 to c++14
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
    strTo = utf8conv.to_bytes(wstr);
#endif  // __cplusplus
#endif  // _WIN32
    return strTo;
}
std::wstring fromUtf8(const std::string &str)
{
    if (str.empty())
        return {};
    std::wstring wstrTo;
#ifdef _WIN32
    int nSizeNeeded =
        ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstrTo.resize(nSizeNeeded, 0);
    ::MultiByteToWideChar(
        CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], nSizeNeeded);
#else  // _WIN32
#if __cplusplus < 201103L || __cplusplus >= 201703L
    // Note: Introduced in c++11 and deprecated with c++17.
    // Revert to C99 code since there no replacement yet
    wstrTo.resize(str.length(), 0);
    locale_t utf8 = newlocale(LC_ALL_MASK, "en_US.UTF-8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.utf-8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.UTF8", NULL);
    if (!utf8)
        utf8 = newlocale(LC_ALL_MASK, "C.utf8", NULL);
    auto nLen = mbstowcs_l(&wstrTo[0], str.c_str(), wstrTo.length(), utf8);
    wstrTo.resize(nLen);
    freelocale(utf8);
#else   // c++11 to c++14
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
    try
    {
        wstrTo = utf8conv.from_bytes(str);
    }
    catch (...)  // Should never fail if str valid UTF-8
    {
    }
#endif  // __cplusplus
#endif  // _WIN32
    return wstrTo;
}

std::wstring toWidePath(const std::string &strUtf8Path)
{
    auto wstrPath{fromUtf8(strUtf8Path)};
#ifdef _WIN32
    // Not needed: normalize path (just replaces '/' with '\')
    std::replace(wstrPath.begin(), wstrPath.end(), L'/', L'\\');
#endif  // _WIN32
    return wstrPath;
}

std::string fromWidePath(const std::wstring &wstrPath)
{
#ifdef _WIN32
    auto srcPath{wstrPath};
    // Not needed: to portable path (just replaces '\' with '/')
    std::replace(srcPath.begin(), srcPath.end(), L'\\', L'/');
#else   // _WIN32
    auto &srcPath{wstrPath};
#endif  // _WIN32
    return toUtf8(srcPath);
}

bool verifySslName(const std::string &certName, const std::string &hostname)
{
    if (certName.find('*') == std::string::npos)
    {
        return certName == hostname;
    }

    size_t firstDot = certName.find('.');
    size_t hostFirstDot = hostname.find('.');
    size_t pos, len, hostPos, hostLen;

    if (firstDot != std::string::npos)
    {
        pos = firstDot + 1;
    }
    else
    {
        firstDot = pos = certName.size();
    }

    len = certName.size() - pos;

    if (hostFirstDot != std::string::npos)
    {
        hostPos = hostFirstDot + 1;
    }
    else
    {
        hostFirstDot = hostPos = hostname.size();
    }

    hostLen = hostname.size() - hostPos;

    // *. in the beginning of the cert name
    if (certName.compare(0, firstDot, "*") == 0)
    {
        return certName.compare(pos, len, hostname, hostPos, hostLen) == 0;
    }
    // * in the left most. but other chars in the right
    else if (certName[0] == '*')
    {
        // compare if `hostname` ends with `certName` but without the leftmost
        // should be fine as domain names can't be that long
        intmax_t hostnameIdx = hostname.size() - 1;
        intmax_t certNameIdx = certName.size() - 1;
        while (hostnameIdx >= 0 && certNameIdx != 0)
        {
            if (hostname[hostnameIdx] != certName[certNameIdx])
            {
                return false;
            }
            hostnameIdx--;
            certNameIdx--;
        }
        if (certNameIdx != 0)
        {
            return false;
        }
        return true;
    }
    // * in the right of the first dot
    else if (firstDot != 0 && certName[firstDot - 1] == '*')
    {
        if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0)
        {
            return false;
        }
        for (size_t i = 0;
             i < hostFirstDot && i < firstDot && certName[i] != '*';
             i++)
        {
            if (hostname[i] != certName[i])
            {
                return false;
            }
        }
        return true;
    }
    // else there's a * in  the middle
    else
    {
        if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0)
        {
            return false;
        }
        for (size_t i = 0;
             i < hostFirstDot && i < firstDot && certName[i] != '*';
             i++)
        {
            if (hostname[i] != certName[i])
            {
                return false;
            }
        }
        intmax_t hostnameIdx = hostFirstDot - 1;
        intmax_t certNameIdx = firstDot - 1;
        while (hostnameIdx >= 0 && certNameIdx >= 0 &&
               certName[certNameIdx] != '*')
        {
            if (hostname[hostnameIdx] != certName[certNameIdx])
            {
                return false;
            }
            hostnameIdx--;
            certNameIdx--;
        }
        return true;
    }

    assert(false && "This line should not be reached in verifySslName");
    // should not reach
    return certName == hostname;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

std::string tlsBackend()
{
    return TOSTRING(TRANTOR_TLS_PROVIDER);
}
#undef TOSTRING
#undef STRINGIFY

#if !defined(USE_BOTAN) && !defined(USE_OPENSSL)
Hash128 md5(const void *data, size_t len)
{
    MD5_CTX ctx;
    trantor_md5_init(&ctx);
    trantor_md5_update(&ctx, (const unsigned char *)data, len);
    Hash128 hash;
    trantor_md5_final(&ctx, (unsigned char *)&hash);
    return hash;
}

Hash160 sha1(const void *data, size_t len)
{
    SHA1_CTX ctx;
    TrantorSHA1Init(&ctx);
    TrantorSHA1Update(&ctx, (const unsigned char *)data, len);
    Hash160 hash;
    TrantorSHA1Final((unsigned char *)&hash, &ctx);
    return hash;
}

Hash256 sha256(const void *data, size_t len)
{
    SHA256_CTX ctx;
    trantor_sha256_init(&ctx);
    trantor_sha256_update(&ctx, (const unsigned char *)data, len);
    Hash256 hash;
    trantor_sha256_final(&ctx, (unsigned char *)&hash);
    return hash;
}

Hash256 sha3(const void *data, size_t len)
{
    Hash256 hash;
    trantor_sha3((const unsigned char *)data, len, &hash, sizeof(hash));
    return hash;
}
#endif

std::string toHexString(const void *data, size_t len)
{
    std::string str;
    str.resize(len * 2);
    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = ((const unsigned char *)data)[i];
        str[i * 2] = "0123456789ABCDEF"[c >> 4];
        str[i * 2 + 1] = "0123456789ABCDEF"[c & 0xf];
    }
    return str;
}

/**
 * @brief Generates `size` random bytes from the systems random source and
 * stores them into `ptr`.
 */
static bool systemRandomBytes(void *ptr, size_t size)
{
#if defined(__BSD__) || defined(__APPLE__)
    arc4random_buf(ptr, size);
    return true;
#elif defined(__linux__) && \
    ((defined(__GLIBC__) && \
      (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))))
    return getentropy(ptr, size) != -1;
#elif defined(_WIN32)  // Windows
    return RtlGenRandom(ptr, (ULONG)size);
#elif defined(__unix__) || defined(__HAIKU__)
    // fallback to /dev/urandom for other/old UNIX
    thread_local std::unique_ptr<FILE, std::function<void(FILE *)> > fptr(
        fopen("/dev/urandom", "rb"), [](FILE *ptr) {
            if (ptr != nullptr)
                fclose(ptr);
        });
    if (fptr == nullptr)
    {
        LOG_FATAL << "Failed to open /dev/urandom for randomness";
        abort();
    }
    if (fread(ptr, 1, size, fptr.get()) != 0)
        return true;
#endif
    return false;
}

struct RngState
{
    Hash160 seed;
    int64_t milli;
    int32_t garbage;
    int counter;
};

bool secureRandomBytes(void *data, size_t len)
{
#if defined(USE_OPENSSL)
    return RAND_bytes((unsigned char *)data, len) == 1;
#elif defined(USE_BOTAN)
    thread_local Botan::AutoSeeded_RNG rng;
    rng.randomize((unsigned char *)data, len);
    return true;
#else
    // CSPRNG proposed by Dan Kaminsky in his DEFCON 22 talk. This makes us use
    // up LESS system entropy With some modifications to make it suitable for
    // trantor's codebase. (RIP Dan Kaminsky. That talk was epic.)
    // https://youtu.be/xneBjc8z0DE?t=2250
    namespace chrono = std::chrono;
    static_assert(sizeof(RngState) < 64,
                  "RngState must be less then SHA1 block size");

    thread_local int useCount = 0;
    thread_local RngState state;
    // reseed every once in a while
    if (useCount == 0)
    {
        if (!systemRandomBytes(&state.seed, sizeof(state.seed)))
            return false;
    }
    useCount = (useCount + 1) % 1024;

    auto now = chrono::steady_clock::now();
    // the proposed algorithm uses the time in nanoseconds, but C++ does not
    // guarantee that so we use milliseconds instead. We use additional entropy
    // from the stack address to compensate for the loss of entropy. (since
    // querying CPU performance counters is a bit out of scope for trantor)
    state.milli = chrono::time_point_cast<chrono::milliseconds>(now)
                      .time_since_epoch()
                      .count();
    // this lives on the stack, so each call _may_ be different. This code
    // works on both 32-bit and 64-bit systems. As well as big-endian and
    // little-endian systems.
    void *stack_ptr = &now;
    uint32_t *stack_ptr32 = (uint32_t *)&stack_ptr;
    uint32_t garbage = *stack_ptr32;
    static_assert(sizeof(void *) >= sizeof(uint32_t), "pointer size too small");
    for (size_t i = 1; i < sizeof(void *) / sizeof(uint32_t); i++)
        garbage ^= stack_ptr32[i];
    state.garbage = garbage;

    // generate the random data as described in the talk
    for (size_t i = 0; i < len / sizeof(Hash160); i++)
    {
        auto hash = sha1(&state, sizeof(state));
        memcpy((char *)data + i * sizeof(hash), &hash, sizeof(hash));
        state.counter++;
    }
    if (len % sizeof(Hash160) != 0)
    {
        auto hash = sha1(&state, sizeof(state));
        memcpy((char *)data + len - len % sizeof(hash),
               &hash,
               len % sizeof(hash));
        state.counter++;
    }
    return true;
#endif
}

}  // namespace utils
}  // namespace trantor
