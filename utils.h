#pragma once
#include <charconv>
#include <cstdint>

/**
 * Cast a T value to string. And will fill all the string into a_buffer_start
 * Fot example, we want to cast an integer whose value is 10 to string into 4 digits.
 * Then we can invoke this API like this: format_number_with_padding( buffer_start, buffer_end,
 * 10, 4). Then the result is: 0010.
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
