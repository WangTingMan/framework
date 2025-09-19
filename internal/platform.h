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

#include "framework/framework_export.h"

namespace framework
{

FRAMEWORK_EXPORT void set_thread_name( const std::string& a_name );

FRAMEWORK_EXPORT uint64_t get_time_stamp();

FRAMEWORK_EXPORT uint64_t get_current_thread_id();

FRAMEWORK_EXPORT std::u8string convert( std::string const& a_source );

FRAMEWORK_EXPORT std::string convert( std::u8string const& a_source );

FRAMEWORK_EXPORT std::wstring convert_to_wstring( std::string a_str );

FRAMEWORK_EXPORT std::string convert_to_string( std::wstring a_str );

FRAMEWORK_EXPORT void base64_encode( char const* a_buffer, uint16_t a_size, std::string* output );

FRAMEWORK_EXPORT bool base64_decode( const std::string& input, std::vector<char>& output );

FRAMEWORK_EXPORT std::vector<std::string> split_string( std::string const& a_source, char a_split );

FRAMEWORK_EXPORT std::string trim_string( std::string const& a_source );

FRAMEWORK_EXPORT std::string current_call_stack();

}

