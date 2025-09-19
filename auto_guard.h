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
#include <functional>

namespace framework
{

class auto_guard
{

public:

    auto_guard
        (
        std::function<void()> a_entry_fun,
        std::function<void()> a_exit_fun
        )
        : m_exit_fun( a_exit_fun )
    {
        a_entry_fun();
    }

    auto_guard( std::function<void()> a_exit_fun )
        : m_exit_fun( a_exit_fun )
    {

    }

    ~auto_guard()
    {
        if( m_exit_fun )
        {
            m_exit_fun();
        }
    }

    auto_guard( const auto_guard& ) = delete;
    auto_guard& operator=( const auto_guard& ) = delete;

private:

    std::function<void()> m_exit_fun;
};

}

