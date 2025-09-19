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

#include "timer_control_block.h"
#include "timer_module.h"
#include "abstract_task.h"
#include "executable_task.h"
#include "framework_manager.h"
#include "log_util.h"

#include <functional>

#ifdef DEBUG_TIMER_MODULE
#define LogTimerDebug LogUtilInfo
#else
#define LogTimerDebug LogUtilIgnore
#endif

namespace framework
{

timer_control_block::timer_control_block()
{

}

void timer_control_block::set_interval( uint32_t a_interval )
{
    m_milliseconds = a_interval;
}

void timer_control_block::timer_triggered()
{
    m_triggered_times++;
    m_timeToExecute = m_milliseconds * ( m_triggered_times + 1 ) + m_timer_start_time;
    LogTimerDebug() << "timer: " << m_name << " m_trigger_times = " << m_trigger_times <<
        ", m_triggered_times = " << m_triggered_times << ", next trigger time: " <<
        timer_module::to_booting_time_stamp( m_timeToExecute );
    if( m_trigger_times > 0 && m_triggered_times >= m_trigger_times )
    {
        LogTimerDebug() << "Going to delete timer: " << m_name;
        std::shared_ptr<executable_task> task;
        int id = m_timer_id;
        auto fun = [id]()->bool
        {
            std::shared_ptr<framework::abstract_module> _module =
                framework::framework_manager::get_instance().get_module_manager().
                get_module( framework::timer_module::s_timer_module_name );
            std::shared_ptr<framework::timer_module> timer_module =
                std::dynamic_pointer_cast< framework::timer_module >( _module );
            timer_module->undregister_timer( id );
            return false;
        };
        task = std::make_shared<executable_task>( fun );
        task->set_target_module( abstract_module::s_timer_module_name );
        task->set_source_module( abstract_module::s_timer_module_name );
        m_callback = []( uint32_t, std::string )
            {
                return false;
            };
        framework_manager::get_instance().get_thread_manager().post_task( task );
    }
}

void timer_control_block::start_schedule()
{
    m_timer_start_time = timer_module::get_system_booting_time();
    m_timeToExecute = m_milliseconds + m_timer_start_time;
}

uint32_t timer_control_block::get_remain_trigger_timers()
{
    if( 0x00 == m_trigger_times )
    {
        return 0xFFFF;
    }

    return m_trigger_times - m_triggered_times;
}

}
