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

#include "abstract_module.h"
#include "executable_task.h"
#include "framework_manager.h"
#include "log_util.h"
#include "module_task_handler.h"
#include "general_seq_task_runner_module.h"

#include <atomic>

namespace framework
{

static std::atomic_uint16_t s_next_module_task_handler_id{ 0 };

module_task_handler::module_task_handler( std::string a_task_handler_name )
{
    if( a_task_handler_name.empty() )
    {
        uint16_t id_value = s_next_module_task_handler_id.fetch_add( 1 );
        a_task_handler_name.append( "default_module_task_handler_" ).append( std::to_string( id_value ) );
    }

    m_task_schedule_helper_module_name = a_task_handler_name;
    auto task_schedule_helper_module = std::make_shared<general_seq_task_runner_module>( a_task_handler_name );
    task_schedule_helper_module->initialize();
    framework_manager::get_instance().get_module_manager().add_new_module( task_schedule_helper_module );
    LogUtilInfo() << "Add new module " << a_task_handler_name;
}

module_task_handler::~module_task_handler()
{
    framework_manager::get_instance().get_module_manager().remove_module( m_task_schedule_helper_module_name );
}

void module_task_handler::handle( std::shared_ptr<abstract_task> a_task )
{
    auto route_task = std::make_shared<executable_task>();
    route_task->set_fun( std::bind( &module_task_handler::execute, this, std::move( a_task ) ),
        m_task_schedule_helper_module_name );
    framework_manager::get_instance().get_thread_manager().post_task( route_task );
}

std::optional<int> module_task_handler::get_current_executing_thread_id()const
{
    uint64_t id = 0;
    id = framework_manager::get_instance().get_thread_manager()
        .get_scheduled_thread_id( m_task_schedule_helper_module_name );
    return static_cast<int>( id );
}

void module_task_handler::execute( std::shared_ptr<abstract_task> a_task )
{
    if( a_task->get_target_module() == abstract_module::s_task_runner_module_name ||
        a_task->get_target_module().empty() )
    {
        auto runnable_task = std::dynamic_pointer_cast<executable_task>( a_task );
        if( runnable_task )
        {
            runnable_task->run_task();
            return;
        }
        else
        {
            LogUtilError() << "Should not go to here!";
        }
        return;
    }

    auto detail_module = framework_manager::get_instance()
        .get_module_manager().get_module( a_task->get_target_module() );
    if( detail_module )
    {
        detail_module->handle_task( a_task );
    }
    else
    {
        LogUtilError() << "No such module: " << a_task->get_target_module();
    }
}

}
