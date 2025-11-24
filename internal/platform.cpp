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

#include <algorithm>
#include <ctime>
#include <chrono>
#include <ranges>
#include <vector>

#if __has_include( <stacktrace> )
#include <stacktrace>
#endif

#ifdef WINDOWS_OS
#include <windows.h>
// The SetThreadDescription API was brought in version 1607 of Windows 10.
typedef HRESULT( WINAPI* SetThreadNameDescription )( HANDLE hThread,
   PCWSTR lpThreadDescription );
#endif

#if defined(ANDROID_OS) || defined(LINUX_OS)
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#endif

#include "../log_util.h"

namespace framework
{

std::wstring convert_to_wstring( std::string a_str )
{
    std::wstring ret_str;

#ifdef WINDOWS_OS
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
    return ret_str;
#endif

    LogUtilFatal() << "No implementation!";
    return ret_str;
}

std::string convert_to_string( std::wstring a_str )
{
    std::string str;

#ifdef WINDOWS_OS
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
    return str;
#endif

    LogUtilFatal() << "No implementation!";
    return str;
}

#ifdef WINDOWS_OS
typedef struct tagTHREADNAME_INFO {
    DWORD dwType;  // Must be 0x1000.
    LPCSTR szName;  // Pointer to name (in user addr space).
    DWORD dwThreadID;  // Thread ID (-1=caller thread).
    DWORD dwFlags;  // Reserved for future use, must be zero.
} THREADNAME_INFO;
static void set_current_thread_name( const char* a_name )
{
    const DWORD kVCThreadNameException = 0x406D1388;
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = a_name;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;
    __try {
        RaiseException( kVCThreadNameException, 0, sizeof( info ) / sizeof( DWORD ),
            reinterpret_cast<DWORD_PTR*>(&info) );
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
}
#endif

void set_thread_name( const std::string& a_name )
{

#ifdef WINDOWS_OS
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
    else
    {
        set_current_thread_name( a_name.c_str() );
        return;
    }
#endif

#if defined(ANDROID_OS) || defined(LINUX_OS)
    // Like linux, on android we can get the thread names to show up in the
    // debugger by setting the process name for the LWP.
    // We don't want to do this for the main thread because that would rename
    // the process, causing tools like killall to stop working.
    if (get_current_thread_id() == getpid())
        return;

    // Set the name for the LWP (which gets truncated to 15 characters).
    int err = prctl( PR_SET_NAME, name.c_str() );
    if (err < 0 && errno != EPERM)
    {
        LogUtilError() << "cannot set current thread's name";
    }
    return;
#endif

    LogUtilFatal() << "No implementation!";
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
#if defined(MACOSX)
    return pthread_mach_thread_np( pthread_self() );
#elif defined(LINUX_OS)
    return syscall( __NR_gettid );
#elif defined(ANDROID_OS)
    return gettid();
#elif defined(FUCHSIA_OS)
    return zx_thread_self();
#elif defined(SOLARIS_OS) || defined(QNX)
    return pthread_self();
#elif defined(NACL_OS) && defined(__GLIBC__)
    return pthread_self();
#elif defined(NACL_OS) && !defined(__GLIBC__)
    // Pointers are 32-bits in NaCl.
    return reinterpret_cast<int32_t>(pthread_self());
#elif defined(POSIX_OS) && defined(AIX_OS)
    return pthread_self();
#elif defined(POSIX_OS) && !defined(AIX_OS)
    return reinterpret_cast<int64_t>(pthread_self());
#elif defined(WINDOWS_OS)
    static thread_local uint64_t tid = ::GetCurrentThreadId();
    return tid;
#else
    LogUtilFatal() << "No implementation!";
#endif
}

std::u8string convert( std::string const& a_source )
{
#ifdef WINDOWS_OS
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
    LogUtilFatal() << "No implementation!";
}

std::string convert( std::u8string const& a_source )
{
#if defined(WINDOWS_OS)
    // UTF-8 -> UTF-16
    int wlen = MultiByteToWideChar( CP_UTF8, 0,
        reinterpret_cast<const char*>(a_source.c_str()),
        a_source.size(), nullptr, 0 );
    std::wstring wstr( wlen, 0 );
    MultiByteToWideChar( CP_UTF8, 0,
        reinterpret_cast<const char*>(a_source.c_str()),
        a_source.size(), wstr.data(), wlen );

    int mlen = WideCharToMultiByte( CP_ACP, 0, wstr.c_str(), wlen, nullptr, 0, nullptr, nullptr );
    std::string str( mlen, 0 );
    WideCharToMultiByte( CP_ACP, 0, wstr.c_str(), wlen, str.data(), mlen, nullptr, nullptr );
    return str;
#elif defined(__linux__) || defined(__APPLE__)
    reinterpret_cast<const char*>(u8str.data());
#else 
    LogUtilFatal() << "No implementation!";
    return std::string();
#endif
}

std::string current_call_stack()
{
    std::string call_stack;

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

    return call_stack;
}

void base64_encode( char const* a_buffer, uint16_t a_size, std::string* output )
{
    LogUtilFatal() << "No implementation!";
}

bool base64_decode( const std::string& input, std::vector<char>& output )
{
    LogUtilFatal() << "No implementation!";
    return false;
}

}

