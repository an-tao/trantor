/**
 *
 *  @file Utilities.h
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

#pragma once

#include <trantor/exports.h>
#include <string>

namespace trantor
{
/**
 * @brief Trantor helper functions.
 *
 */
namespace utils
{
/**
 * @brief Convert a wide string to a UTF-8.
 * @details UCS2 on Windows, UTF-32 on Linux & Mac
 *
 * @param str String to convert
 *
 * @return converted string.
 */
TRANTOR_EXPORT std::string toUtf8(const std::wstring &wstr);
/**
 * @brief Convert a UTF-8 string to a wide string.
 * @details UCS2 on Windows, UTF-32 on Linux & Mac
 *
 * @param str String to convert
 *
 * @return converted string.
 */
TRANTOR_EXPORT std::wstring fromUtf8(const std::string &str);

/**
 * @details Convert a wide string path with arbitrary directory separators
 * to a UTF-8 portable path for use with trantor.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, backslash directory separators are converted to slash to
 * keep portable paths.
 *
 * @remarks On other OSes, backslashes are not converted to slash, since they
 * are valid characters for directory/file names.
 *
 * @param strPath Wide string path.
 *
 * @return std::string UTF-8 path, with slash directory separator.
 */
TRANTOR_EXPORT std::string fromWidePath(const std::wstring &strPath);
/**
 * @details Convert a UTF-8 path with arbitrary directory separator to a wide
 * string path.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, slash directory separators are converted to backslash.
 * Although it accepts both slash and backslash as directory separator in its
 * API, it is better to stick to its standard.

 * @remarks On other OSes, slashes are not converted to backslashes, since they
 * are not interpreted as directory separators and are valid characters for
 * directory/file names.
 *
 * @param strUtf8Path Ascii path considered as being UTF-8.
 *
 * @return std::wstring path with, on windows, standard backslash directory
 * separator to stick to its standard.
 */
TRANTOR_EXPORT std::wstring toWidePath(const std::string &strUtf8Path);

// Helpers for multi-platform development
// OS dependent
#if defined(_WIN32) && !defined(__MINGW32__)
/**
 * @details Convert an UTF-8 path to a native path.
 *
 * This is a helper for Windows and multi-platform projects.
 *
 * Converts the path from UTF-8 to wide string and replaces slash directory
 * separators with backslash.
 *
 * @remarks Although it accepts both slash and backslash as directory
 * separators in its API, it is better to stick to its standard.
 *
 * @param strPath UTF-8 path.
 *
 * @return Native path suitable for the Windows unicode API.
 */
inline std::wstring toNativePath(const std::string &strPath)
{
    return toWidePath(strPath);
}
/**
 * @details Convert an UTF-8 path to a native path.
 *
 * This is a helper for non-Windows OSes for multi-platform projects.
 *
 * Does nothing.
 *
 * @param strPath UTF-8 path.
 *
 * @return \p strPath.
 */
inline const std::wstring &toNativePath(const std::wstring &strPath)
{
    return strPath;
}
#else   // __WIN32
/**
 * @details Convert an UTF-8 path to a native path.
 *
 * This is a helper for non-Windows OSes for multi-platform projects.
 *
 * Does nothing.
 *
 * @param strPath UTF-8 path.
 *
 * @return \p strPath.
 */
inline const std::string &toNativePath(const std::string &strPath)
{
    return strPath;
}
/**
 * @details Convert an wide string path to a UTF-8 path.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, backslash directory separators are converted to slash to
 * keep portable paths.

 * @warning On other OSes, backslashes are not converted to slash, since they
 * are valid characters for directory/file names.
 *
 * @param strPath wide string path.
 *
 * @return Generic path, with slash directory separators
 */
inline std::string toNativePath(const std::wstring &strPath)
{
    return fromWidePath(strPath);
}
#endif  // _WIN32
/**
 * @note NoOP on all OSes
 */
inline const std::string &fromNativePath(const std::string &strPath)
{
    return strPath;
}
/**
 * @note fromWidePath() on all OSes
 */
// Convert on all systems
inline std::string fromNativePath(const std::wstring &strPath)
{
    return fromWidePath(strPath);
}

}  // namespace utils

}  // namespace trantor
