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
#include "abstract_task.h"
#include "abstract_module.h"

namespace framework
{

class executable_task : public abstract_task
{

public:

    executable_task( std::function<bool()> a_fun )
    {
        set_task_type( task_type::executable_task );
        m_target_name = abstract_module::s_task_runner_module_name;
        m_task = a_fun;
    }

    executable_task()
    {
    }

    void set_fun
        (
        std::function<void()> a_fun,
        std::string a_target_module
        )
    {
        if( !m_task )
        {
            set_task_type( task_type::executable_task );
            m_target_name = a_target_module;
            if( a_target_module.empty() )
            {
                m_target_name = abstract_module::s_task_runner_module_name;
            }
            m_task_no_return = a_fun;
        }
    }

    /**
    * return true to exit current thread
    */
    bool run_task()
    {
        if( m_task )
        {
            return m_task();
        }

        if( m_task_no_return )
        {
            m_task_no_return();
            return false;
        }

        return false;
    }

    bool operator()()
    {
        return run_task();
    }

private:

    std::function<bool()> m_task;
    std::function<void()> m_task_no_return;

};

}

