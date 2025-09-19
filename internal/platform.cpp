/*
  Copyright (c) 2009-2025

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "platform.h"

#if __has_include(<base/threading/platform_thread.h>)
#ifndef HAS_LIBCHROME_LIBRARAY
#define HAS_LIBCHROME_LIBRARAY
#endif
#endif

#ifdef HAS_LIBCHROME_LIBRARAY
#include <base/threading/platform_thread.h>
#include <base/strings/sys_string_conversions.h>
#include <base/strings/utf_string_conversions.h>
#include <base/base64.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <base/debug/stack_trace.h>
#pragma comment(lib, "libChromeBase.lib")
#endif

#include <ctime>
#include <chrono>

#if __has_include( <stacktrace> )
#include <stacktrace>
#endif

#ifdef _WIN32
#include <windows.h>
// The SetThreadDescription API was brought in version 1607 of Windows 10.
typedef HRESULT( WINAPI* SetThreadNameDescription )( HANDLE hThread,
   PCWSTR lpThreadDescription );
#endif

namespace framework
{

std::wstring convert_to_wstring( std::string a_str )
{
    std::wstring ret_str;
#ifdef HAS_LIBCHROME_LIBRARAY
    ret_str = base::SysNativeMBToWide( a_str );
#else

#ifdef _WIN32
    if( a_str.empty() )
        return std::wstring();

    int mb_length = static_cast< int >( a_str.length() );
    // Compute the length of the buffer.
    int charcount = MultiByteToWideChar( CP_ACP, 0,
        a_str.data(), mb_length, NULL, 0 );
    if( charcount == 0 )
        return std::wstring();

    ret_str.resize( charcount );
    MultiByteToWideChar( CP_ACP, 0, a_str.data(), mb_length, &ret_str[0], charcount );
#endif

#endif
    return ret_str;
}

std::string convert_to_string( std::wstring a_str )
{
    std::string str;
#ifdef HAS_LIBCHROME_LIBRARAY
    str = base::SysWideToNativeMB( a_str );
#else

#ifdef _WIN32
    int wide_length = static_cast< int >( a_str.length() );
    if( wide_length == 0 )
        return std::string();

    // Compute the length of the buffer we'll need.
    int charcount = WideCharToMultiByte( CP_ACP, 0, a_str.data(), wide_length,
        NULL, 0, NULL, NULL );
    if( charcount == 0 )
        return std::string();

    str.resize( charcount );
    WideCharToMultiByte( CP_ACP, 0, a_str.data(), wide_length,
        &str[0], charcount, NULL, NULL );
#endif

#endif
    return str;
}

void set_thread_name( const std::string& a_name )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    base::PlatformThread::SetName( a_name );
#else

#ifdef _WIN32
    // The SetThreadDescription API works even if no debugger is attached.
    static auto set_thread_description_func =
        reinterpret_cast< SetThreadNameDescription >( ::GetProcAddress(
            ::GetModuleHandle( L"Kernel32.dll" ), "SetThreadDescription" ) );
    if( set_thread_description_func )
    {
        std::wstring thread_name = convert_to_wstring( a_name );
        set_thread_description_func( ::GetCurrentThread(),
            thread_name.c_str() );
        return;
    }
#endif

#endif
}

uint64_t get_time_stamp()
{
    uint64_t ret = 0;
    struct timespec ts;
    int r = timespec_get( &ts, TIME_UTC );
    ret = ( ( uint64_t )ts.tv_sec * 1000000L ) +
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::nanoseconds( ts.tv_nsec ) ).count();
    return ret;
}

uint64_t get_current_thread_id()
{
#ifdef _WIN32
    static thread_local uint64_t tid = ::GetCurrentThreadId();
#endif
    return tid;
}

std::u8string convert( std::string const& a_source )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    std::wstring w_str = base::SysNativeMBToWide( a_source );
    std::string utf8_str = base::SysWideToUTF8( w_str );

    std::u8string::value_type const* _str = nullptr;
    _str = reinterpret_cast< std::u8string::value_type const* >( utf8_str.c_str() );
    std::u8string str( _str, utf8_str.size() + 1 );

    return str;
#else

#ifdef _WIN32
    std::u8string str;
    std::wstring w_str = convert_to_wstring( a_source );
    int wide_length = static_cast< int >( w_str.length() );
    if( wide_length == 0 )
        return str;

    // Compute the length of the buffer we'll need.
    int charcount = WideCharToMultiByte( CP_UTF8, 0, w_str.data(), wide_length,
        NULL, 0, NULL, NULL );
    if( charcount == 0 )
        return str;

    std::string utf8_str;
    utf8_str.resize( charcount );
    WideCharToMultiByte( CP_UTF8, 0, w_str.data(), wide_length,
        &utf8_str[0], charcount, NULL, NULL );
    std::u8string::value_type const* _str = nullptr;
    _str = reinterpret_cast< std::u8string::value_type const* >( utf8_str.c_str() );
    str = std::u8string( _str, utf8_str.size() + 1 );
    return str;
#endif

#endif
}

std::string convert( std::u8string const& a_source )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    const char* src = reinterpret_cast< const char* >( a_source.c_str() );
    std::wstring wstr;
    bool conver_re = base::UTF8ToWide( src, a_source.size(), &wstr );
    std::string r = base::SysWideToNativeMB( wstr );
    r.push_back( '\0' );
    return r.c_str();
#else

#ifdef _WIN32
    const char* src = reinterpret_cast< const char* >( a_source.c_str() );
    std::wstring wstr;
#endif

#endif
}

std::string current_call_stack()
{
    std::string call_stack;
#ifdef HAS_LIBCHROME_LIBRARAY
    std::stringstream ss;
    std::string logstr;
    base::debug::StackTrace trace;
    trace.OutputToStream(&ss);
    call_stack = ss.str();
#else

#if __has_include(<stacktrace>)
#ifdef __cpp_lib_stacktrace
    std::basic_stacktrace calls_ =  std::stacktrace::current();
    for (auto& ele : calls_)
    {
        std::string frame;
        frame.append( ele.source_file() ).append( ":" ).append( std::to_string( ele.source_line() ) );
        frame.append( "\n" );
        call_stack.append( frame );
    }
#endif
#endif

#endif
    return call_stack;
}

void base64_encode( char const* a_buffer, uint16_t a_size, std::string* output )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    base::Base64Encode( a_buffer, a_size, output );
#endif
}

bool base64_decode( const std::string& input, std::vector<char>& output )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    return base::Base64Decode( input, output );
#else
    return false;
#endif
}

std::vector<std::string> split_string( std::string const& a_source, char a_split )
{
#ifdef HAS_LIBCHROME_LIBRARAY
    std::string splitor;
    splitor.push_back( a_split );
    return base::SplitString( a_source, splitor, base::WhitespaceHandling::TRIM_WHITESPACE,
        base::SplitResult::SPLIT_WANT_NONEMPTY );
#else
    return std::vector<std::string>();
#endif
}

std::string trim_string( std::string const& a_source )
{
    std::string out;
#ifdef HAS_LIBCHROME_LIBRARAY
    base::StringPiece str = base::TrimWhitespaceASCII( a_source, base::TrimPositions::TRIM_ALL );
    out = str.as_string();
#endif
    return out;
}

}

