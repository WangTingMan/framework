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

#include <ranges>

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

}
