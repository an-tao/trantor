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
#include <algorithm>
#else  // _WIN32
#if __cplusplus < 201103L || __cplusplus >= 201703L
#include <stdlib.h>
#include <locale.h>
#else  // __cplusplus
#include <locale>
#include <codecvt>
#endif  // __cplusplus
#endif  // _WIN32

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

    // should not reach
    return certName == hostname;
}

}  // namespace utils
}  // namespace trantor
