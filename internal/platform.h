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

#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "../framework_export.h"

// A set of macros to use for platform detection.
#if defined(__native_client__)
// __native_client__ must be first, so that other OS_ defines are not set.
#define NACL_OS 1
// OS_NACL comes in two sandboxing technology flavors, SFI or Non-SFI.
// PNaCl toolchain defines __native_client_nonsfi__ macro in Non-SFI build
// mode, while it does not in SFI build mode.
#if defined(__native_client_nonsfi__)
#define NACL_NONSFI_OS
#else
#define NACL_SFI_OS
#endif
#elif defined(__APPLE__)
// only include TargetConditions after testing ANDROID as some android builds
// on mac don't have this header available and it's not needed unless the target
// is really mac/ios.
#include <TargetConditionals.h>
#define MACOSX 1
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define IOS_OS 1
#endif  // defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#elif defined(__linux__)
#define LINUX_OS 1
// include a system header to pull in features.h for glibc/uclibc macros.
#include <unistd.h>
#if defined(__GLIBC__) && !defined(__UCLIBC__)
// we really are using glibc, not uClibc pretending to be glibc
#define LIBC_GLIBC 1
#endif
#elif defined(_WIN32)
#define WINDOWS_OS 1
#elif defined(__Fuchsia__)
#define FUCHSIA_OS 1
#elif defined(__FreeBSD__)
#define FREEBSD_OS 1
#elif defined(__NetBSD__)
#define NETBSD_OS 1
#elif defined(__OpenBSD__)
#define OPENBSD_OS 1
#elif defined(__sun)
#define SOLARIS_OS 1
#elif defined(__QNXNTO__)
#define QNX_OS 1
#elif defined(_AIX)
#define AIX_OS 1
#elif defined(__asmjs__)
#define ASMJS_OS
#elif defined(__ANDROID__)
#define ANDROID_OS
#else
#error Please add support for your platform in build/build_config.h
#endif

// For access to standard POSIXish features, use POSIX instead of a
// more specific macro.
#if defined(AIX_OS) || defined(ANDROID_OS) || defined(ASMJS_OS) ||    \
    defined(FREEBSD_OS) || defined(LINUX_OS) || defined(MACOSX_OS) || \
    defined(NACL_OS) || defined(NETBSD_OS) || defined(OPENBSD_OS) ||  \
    defined(QNX_OS) || defined(SOLARIS_OS)
#define POSIX_OS 1
#endif

namespace framework
{

FRAMEWORK_EXPORT void set_thread_name( const std::string& a_name );

FRAMEWORK_EXPORT uint64_t get_time_stamp();

FRAMEWORK_EXPORT uint64_t get_current_thread_id();

FRAMEWORK_EXPORT std::u8string convert( std::string const& a_source );

FRAMEWORK_EXPORT std::string convert( std::u8string const& a_source );

FRAMEWORK_EXPORT std::wstring convert_to_wstring( std::string a_str );

FRAMEWORK_EXPORT std::string convert_to_string( std::wstring a_str );

FRAMEWORK_EXPORT std::string current_call_stack();

}

