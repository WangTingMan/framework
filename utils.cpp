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

#include "utils.h"

#include <array>
#include <ranges>
#include <stdexcept>

namespace framework
{

std::vector<std::string> split_string( std::string const& a_source, char a_split )
{
    std::vector<std::string> result;

    for (auto&& rng : a_source | std::views::split( a_split ))
    {
        result.emplace_back( &*rng.begin(), std::ranges::distance( rng ) );
    }

    return result;
}

std::string trim_string( std::string const& a_source )
{
    const char* whitespace = " \t\n\r\v\f";

    std::string_view sv( a_source );
    size_t start = sv.find_first_not_of( whitespace );
    if (start == std::string_view::npos)
    {
        return std::string();
    }

    size_t end = sv.find_last_not_of( whitespace );
    return std::string( sv.substr( start, end - start + 1 ) );
}

static constexpr char base64_enc_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr std::array<int8_t, 256> make_base64_dec_table()
{
    std::array<int8_t, 256> table{};

    for (int i = 0; i < 256; ++i)
    {
        table[i] = -1;
    }

    for (int i = 0; i < 64; ++i)
    {
        const uint8_t c = static_cast<uint8_t>( base64_enc_table[i] );
        table[c] = static_cast<int8_t>( i );
    }

    return table;
}

static constexpr std::array<int8_t, 256> base64_dec_table = make_base64_dec_table();
static_assert(base64_dec_table['A'] == 0, "compiling checking: A should map to 0");
static_assert(base64_dec_table['z'] == 51, "compiling checking: z should map to 51");
static_assert(base64_dec_table['9'] == 61, "compiling checking: 9 should map to 61");
static_assert(base64_dec_table['+'] == 62, "compiling checking: + should map to62");
static_assert(base64_dec_table['/'] == 63, "compiling checking: / should map to 63");

void base64_encode( char const* a_input, size_t a_input_len, std::string* a_output )
{
    if ( (a_input_len == 0 ) || (a_input == nullptr ) )
    {
        return;
    }

    if (!a_output)
    {
        return;
    }

    const size_t output_len = ((a_input_len + 2) / 3) * 4;
    std::string& output = *a_output;
    output.clear();
    output.reserve( output_len );

    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(a_input);
    size_t remaining = a_input_len;

    while (remaining >= 3)
    {
        output += base64_enc_table[(ptr[0] & 0xFC) >> 2];
        output += base64_enc_table[((ptr[0] & 0x03) << 4) | ((ptr[1] & 0xF0) >> 4)];
        output += base64_enc_table[((ptr[1] & 0x0F) << 2) | ((ptr[2] & 0xC0) >> 6)];
        output += base64_enc_table[ptr[2] & 0x3F];

        ptr += 3;
        remaining -= 3;
    }

    if (remaining > 0)
    {
        output += base64_enc_table[(ptr[0] & 0xFC) >> 2];
        if (remaining == 1)
        {
            output += base64_enc_table[(ptr[0] & 0x03) << 4];
            output += '=';
            output += '=';
        }
        else
        {
            output += base64_enc_table[((ptr[0] & 0x03) << 4) | ((ptr[1] & 0xF0) >> 4)];
            output += base64_enc_table[(ptr[1] & 0x0F) << 2];
            output += '=';
        }
    }

    return;
}

bool base64_decode( const std::string& input, std::vector<char>& output )
{
    const size_t input_len = input.size();
    if (input_len == 0)
    {
        output.clear();
        return true;
    }

    if (input_len % 4 != 0)
    {
        return false;
    }

    size_t pad_count = 0;
    if (input_len > 0)
    {
        if (input.back() == '=') pad_count++;
        if (input_len > 1 && input[input_len - 2] == '=') pad_count++;
    }

    const size_t output_len = (input_len / 4) * 3 - pad_count;
    output.reserve( output_len );

    const char* ptr = input.c_str();
    size_t remaining = input_len;

    while (remaining >= 4)
    {
        const int8_t a = base64_dec_table[static_cast<uint8_t>(ptr[0])];
        const int8_t b = base64_dec_table[static_cast<uint8_t>(ptr[1])];
        const int8_t c = base64_dec_table[static_cast<uint8_t>(ptr[2])];
        const int8_t d = base64_dec_table[static_cast<uint8_t>(ptr[3])];

        if (a == -1 || b == -1 || (c == -1 && ptr[2] != '=') || (d == -1 && ptr[3] != '='))
        {
            return false;
        }

        output.push_back( static_cast<unsigned char>((a << 2) | ((b & 0x30) >> 4)) );
        if (ptr[2] != '=')
        {
            output.push_back( static_cast<unsigned char>(((b & 0x0F) << 4) | ((c & 0x3C) >> 2)) );
        }

        if (ptr[3] != '=')
        {
            output.push_back( static_cast<unsigned char>(((c & 0x03) << 6) | d) );
        }

        ptr += 4;
        remaining -= 4;
    }

    if (output.size() != output_len)
    {
        return false;
    }

    return true;
}

}
