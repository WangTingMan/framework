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
#include "module_manager.h"
#include "thread_manager.h"
#include "information_manager.h"

#include <shared_mutex>

#include "framework_export.h"

namespace framework
{

class FRAMEWORK_EXPORT framework_manager
{

public:

    static framework_manager& get_instance();

    thread_manager& get_thread_manager()
    {
        return m_thread_manager;
    }

    module_manager& get_module_manager()
    {
        return m_module_manager;
    }

    information_manager& get_info_manager()
    {
        return m_info_manager;
    }

    void run
        (
        std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker,
        bool a_occupy_current_thread = false
        );

    void power_up();

    bool is_running()const;

private:

    void init( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker );

    module_manager m_module_manager;
    thread_manager m_thread_manager;
    information_manager m_info_manager;

    mutable std::shared_mutex m_mutex;
    bool m_is_running = false;
};

}

