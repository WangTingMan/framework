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
#include "abstract_info.h"
#include "framework_export.h"

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <string>

namespace framework
{

class base_element;

class FRAMEWORK_EXPORT information_manager
{

public:

    information_manager();

    bool register_information
        (
        std::shared_ptr<abstract_information> a_information
        );

    std::shared_ptr<abstract_information> get_informaion( std::string const& a_name );

    template<typename T>
    std::shared_ptr<T> get_detail_information( std::string const& a_name )
    {
        auto info = get_informaion( a_name );
        return std::dynamic_pointer_cast< T >( info );
    }

private:

    std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<abstract_information>> m_informations;
};

}

