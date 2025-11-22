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
#include <charconv>
#include <cstdint>
#include <string>
#include <vector>

#include "framework/framework_export.h"

namespace framework
{

/**
 * Cast a T value to string. And will fill all the string into a_buffer_start
 * Fot example, we want to cast an integer whose value is 10 to string into 4 digits.
 * Then we can invoke this API like this: format_number_with_padding( buffer_start, buffer_end,
 * 10, 4). Then the result is: 0010.
 * this is a HIGH performance implementation.
 * a_buffer_start: the buffer's start pos
 * a_buffer_end: the buffer;s end pos
 * a_value: the value to cast
 * a_min_digits: how long size it will used.
 */
template <typename T>
bool format_number_with_padding
    (
    char*&  a_buffer_start,
    char*   a_buffer_end,
    T       a_value,
    int     a_min_digits
    )
{
    // determine how many prefix 0
    int padding = 0;
    if( a_min_digits > 0 ) {
        T power = 1;
        for( int i = 0; i < a_min_digits - 1; ++i ) {
            power *= 10;
        }
        while( a_value < power && padding < a_min_digits ) {
            padding++;
            power /= 10;
        }
    }

    // padding 0
    int i = ( (0 == a_value) ? 1 : 0 );
    for( ; i < padding && a_buffer_start < a_buffer_end; i++ ) {
        *a_buffer_start++ = '0';
    }

    // format
    auto result = std::to_chars( a_buffer_start, a_buffer_end, a_value );
    if( result.ec != std::errc() ) {
        return false;
    }
    a_buffer_start = result.ptr;
    return true;
}

FRAMEWORK_EXPORT std::vector<std::string> split_string( std::string const& a_source, char a_split );

FRAMEWORK_EXPORT std::string trim_string( std::string const& a_source );

}
