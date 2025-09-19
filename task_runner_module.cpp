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

#include "task_runner_module.h"
#include "abstract_task.h"
#include "executable_task.h"
#include "log_util.h"
#include "framework_event.h"

namespace framework
{

task_runner_module::task_runner_module()
{
    set_name( s_task_runner_module_name );
    set_module_type( abstract_module::module_type::concurrently_executing );
}

void task_runner_module::initialize()
{
    set_power_status( abstract_module::powering_status::power_on );
}

void task_runner_module::deinitialize()
{
    set_power_status( abstract_module::powering_status::power_off );
}

void task_runner_module::handle_task( std::shared_ptr<abstract_task> a_task )
{
    if( a_task->get_target_module() == s_task_runner_module_name ||
        a_task->get_target_module().empty() )
    {
        auto runnable_task = std::dynamic_pointer_cast< executable_task >( a_task );
        if( runnable_task )
        {
            runnable_task->run_task();
        }
    }
    else
    {
        LogUtilError() << "Received wrong module task. source module:"
            << a_task->get_source_module() << ", target module: "
            << a_task->get_target_module();
    }
}

void task_runner_module::handle_event( std::shared_ptr<framework_event> a_event )
{
    switch( a_event->m_event_type )
    {
    case event_type::power_on:
        set_power_status( abstract_module::powering_status::power_on );
        break;
    case event_type::power_off:
        set_power_status( abstract_module::powering_status::power_off );
        break;
    default:
        break;
    }
}

}

