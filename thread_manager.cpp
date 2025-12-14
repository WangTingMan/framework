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

#include "auto_guard.h"
#include "thread_manager.h"
#include "framework_manager.h"
#include "module_task_handler.h"
#include "timer_module.h"
#include "thread_worker.h"
#include "log_util.h"
#include "executable_task.h"

namespace framework
{

static thread_local std::string s_thread_module_owner;

std::string const& thread_manager::get_current_thread_module_owner()
{
    return s_thread_module_owner;
}

void thread_manager::set_current_thread_module_owner( std::string a_module_name )
{
    s_thread_module_owner = std::move( a_module_name );
}

void thread_manager::run( bool a_occupy_current_thread )
{
    std::shared_ptr<abstract_worker> current_thread_worker;
    if( a_occupy_current_thread )
    {
        current_thread_worker = make_worker();
    }

    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    /**
     * If there is no worker to do work, then recruit some one.
     */
    if( m_idle_worker.empty() )
    {
        m_idle_worker.emplace_back( make_worker() );
        m_idle_worker.emplace_back( make_worker() );
        for( auto& ele : m_idle_worker )
        {
            ele->run( ele, false );
        }
    }

    if( current_thread_worker )
    {
        m_idle_worker.push_back( current_thread_worker );
    }

    if( 0 != m_schedule_timer_id )
    {
        std::shared_ptr<abstract_task> task;
        auto fun = [this]()->bool
        {
            auto _module = framework_manager::get_instance()
                .get_module_manager().get_module( abstract_module::s_timer_module_name );
            auto _timer_module = std::dynamic_pointer_cast< timer_module >( _module );
            auto timer_cb = [this]( uint32_t, std::string )->bool
            {
                schedule_workers();
                return false;
            };
            m_schedule_timer_id = _timer_module->register_timer( timer_cb,
                std::chrono::milliseconds( 2310 ) );
            LogUtilInfo() << "schedule timer registered.";
            return false;
        };
        m_work_need_assign.push_back( std::make_shared<executable_task>( fun ) );
    }

    locker.unlock();

    if( current_thread_worker )
    {
        current_thread_worker->run( current_thread_worker, true );
    }

}

void thread_manager::post_task( std::function<void()> a_tsk )
{
    if( a_tsk )
    {
        auto tsk =  std::make_shared<executable_task>();
        tsk->set_fun( a_tsk, abstract_module::s_task_runner_module_name );
        post_task( tsk );
    }
}

void thread_manager::post_delay_task
    (
    std::chrono::milliseconds a_delay_time,
    std::function<void()> a_tsk
    )
{
    if( !a_tsk )
    {
        return;
    }

    auto timer_module_ = std::dynamic_pointer_cast< framework::timer_module >(
        framework::framework_manager::get_instance().get_module_manager().get_module(
            framework::timer_module::s_timer_module_name ) );
    timer_module_->register_once_timer( [a_tsk]( uint32_t, std::string )
        {
            a_tsk();
        }, a_delay_time );
}

void thread_manager::post_task( std::shared_ptr<abstract_task> a_task )
{
    std::unique_lock<std::recursive_mutex> locker( m_mutex );

    std::string const& _module = a_task->get_target_module();
    if( _module.empty() )
    {
        if( a_task->get_task_type() == task_type::framework_event )
        {
            std::vector<std::shared_ptr<abstract_task>> tasks;
            for( auto& ele : m_modules_shcedule )
            {
                std::shared_ptr<abstract_task> cloned_task = a_task->clone();
                cloned_task->set_target_module( ele.first );
                tasks.emplace_back( std::move( cloned_task ) );
            }
            post_task( std::move( tasks ) );
            return;
        }
        else
        {
            LogUtilInfo() << "Received a task without target module, then dispatch each module.";
        }
    }

    if( !_module.empty() && !m_modules_shcedule.contains( _module ) )
    {
        LogUtilError() << "Such module has not registered: " << _module;
    }

    module_task_cb& cb = m_modules_shcedule[_module];
    switch( cb.module_type_value )
    {
    case abstract_module::module_type::sequence_executing:
        schedule_sequence_task( cb, std::move( a_task ) );
        return;
    case abstract_module::module_type::execute_task_when_post:
        locker.unlock();
        schedule_immediately_task( std::move( a_task ), _module );
        return;
    case abstract_module::module_type::concurrently_executing:
        schedule_concurrently_task( std::move( a_task ) );
        return;
    case abstract_module::module_type::handler_shchedule:
        schedule_handler_task( std::move( a_task ) );
        return;
    default:
        LogUtilError() << "unknown module task type.";
        break;
    }
}

void thread_manager::post_task( std::vector<std::shared_ptr<abstract_task>> a_tasks )
{
    for( auto& ele : a_tasks )
    {
        post_task( ele );
    }
}

void thread_manager::push_idle_worker( std::shared_ptr<abstract_worker> a_worker )
{
    std::lock_guard<std::recursive_mutex> locker( m_mutex );
    bool task_assigned = false;
    for( auto it = m_modules_shcedule.begin(); it != m_modules_shcedule.end(); ++it )
    {
        if( it->second.m_executing_worker == a_worker )
        {
            if( it->second.pending_tasks.empty() )
            {
                it->second.m_executing_worker.reset();
            }
            else
            {
                std::vector<std::shared_ptr<abstract_task>> tasks
                    ( it->second.pending_tasks.begin(), it->second.pending_tasks.end() );
                a_worker->post_task( tasks );
                it->second.pending_tasks.clear();
                task_assigned = true;
            }
        }
        else
        {
            // if no worker doing these work currently, we need find a worker to do
            if( !( it->second.m_executing_worker ) )
            {
                if( !( it->second.pending_tasks.empty() ) )
                {
                    std::shared_ptr<abstract_worker> worker;
                    worker = find_idle_worker();
                    if( worker )
                    {
                        assign_work( worker, it->second.pending_tasks );
                        it->second.pending_tasks.clear();
                        it->second.m_executing_worker = worker;
                    }
                }
            }
        }
    }

    if( task_assigned )
    {
        return;
    }


    if( !m_work_need_assign.empty() )
    {
        // There is a work need to do and assign to a_worker. So do not
        // push it into idle worker list.
        a_worker->post_task( m_work_need_assign.front() );
        m_work_need_assign.erase( m_work_need_assign.begin() );
        return;
    }

    if( std::find( m_idle_worker.begin(), m_idle_worker.end(), a_worker ) == m_idle_worker.end() )
    {
        m_idle_worker.push_back( a_worker );
    }

    auto it = std::find( m_working_worker.begin(), m_working_worker.end(), a_worker );
    if( it != m_working_worker.end() )
    {
        m_working_worker.erase( it );
    }
}

void thread_manager::register_module_type
    (
    abstract_module::module_type a_type,
    std::string a_module_name
    )
{
    module_task_cb cb;
    cb.module_name = a_module_name;
    cb.module_type_value = a_type;

    std::lock_guard<std::recursive_mutex> locker( m_mutex );
    if( !m_modules_shcedule.contains( a_module_name ) )
    {
        m_modules_shcedule[a_module_name] = cb;
    }
    else
    {
        LogUtilInfo() << "Already has " << a_module_name << ", change module tye.";
        m_modules_shcedule[a_module_name].module_type_value = a_type;
    }
}

uint64_t thread_manager::get_scheduled_thread_id( std::string const& a_moudle_name )const
{
    std::lock_guard locker( m_mutex );
    for( auto it = m_modules_shcedule.begin(); it != m_modules_shcedule.end(); ++it )
    {
        if( a_moudle_name == it->first )
        {
            auto& worker = it->second.m_executing_worker;
            if( worker )
            return worker->work_thread_id();
        }
    }
    return 0;
}

void thread_manager::remove_worker( std::shared_ptr<abstract_worker> a_worker )
{
    std::lock_guard<std::recursive_mutex> locker( m_mutex );
    for( auto it = m_idle_worker.begin(); it != m_idle_worker.end(); )
    {
        if( it->get() == a_worker.get() )
        {
            it = m_idle_worker.erase( it );
        }
        else
        {
            ++it;
        }
    }

    for( auto it = m_working_worker.begin(); it != m_working_worker.end(); )
    {
        if( it->get() == a_worker.get() )
        {
            it = m_working_worker.erase( it );
        }
        else
        {
            ++it;
        }
    }

    for( auto it = m_modules_shcedule.begin(); it != m_modules_shcedule.end(); ++it )
    {
        if( it->second.m_executing_worker.get() == a_worker.get() )
        {
            it->second.m_executing_worker.reset();
        }
    }
}

void thread_manager::schedule_workers()
{
    std::lock_guard<std::recursive_mutex> locker( m_mutex );
    if( !m_idle_worker.empty() )
    {
        return;
    }

    if( m_working_worker.size() < s_max_worker_num )
    {
        std::shared_ptr<abstract_worker> worker = make_worker();
        worker->run( worker, false );
        m_idle_worker.push_back( worker );
    }
}

std::shared_ptr<abstract_worker> thread_manager::find_idle_worker()
{
    std::shared_ptr<abstract_worker> worker;
    if( !m_idle_worker.empty() )
    {
        worker = m_idle_worker.front();
        m_idle_worker.erase( m_idle_worker.begin() );
    }
    else
    {
        schedule_workers();
        if( m_idle_worker.empty() )
        {
            return worker;
        }
        else
        {
            worker = m_idle_worker.front();
            m_idle_worker.erase( m_idle_worker.begin() );
        }
    }

    dismiss_long_idle_worker();
    return worker;
}

void thread_manager::assign_work
    (
    std::shared_ptr<abstract_worker>& a_worker,
    std::shared_ptr<abstract_task>& a_task
    )
{
    a_worker->post_task( a_task );
    auto it = std::find( m_idle_worker.begin(), m_idle_worker.end(), a_worker );
    if( it != m_idle_worker.end() )
    {
        m_idle_worker.erase( it );
    }

    auto it_ = std::find( m_working_worker.begin(), m_working_worker.end(), a_worker );
    if( it_ == m_working_worker.end() )
    {
        m_working_worker.push_back( a_worker );
    }
}

void thread_manager::assign_work
    (
    std::shared_ptr<abstract_worker>& a_worker,
    std::list<std::shared_ptr<abstract_task>>& a_task
    )
{
    std::vector<std::shared_ptr<abstract_task>> tasks( a_task.begin(), a_task.end() );
    a_worker->post_task( tasks );

    auto it = std::find( m_idle_worker.begin(), m_idle_worker.end(), a_worker );
    if( it != m_idle_worker.end() )
    {
        m_idle_worker.erase( it );
    }

    auto it_ = std::find( m_working_worker.begin(), m_working_worker.end(), a_worker );
    if( it_ == m_working_worker.end() )
    {
        m_working_worker.push_back( a_worker );
    }
}

void thread_manager::dismiss_long_idle_worker()
{
    if( m_idle_worker.size() > 2 )
    {
        for( auto& ele : m_idle_worker )
        {
            if( ele->is_idle_for_long_time() )
            {
                ele->exit_later();
                return;
            }
        }
    }
}

void thread_manager::schedule_sequence_task
    (
    module_task_cb& a_task_cb,
    std::shared_ptr<abstract_task> a_task
    )
{
    if( a_task_cb.m_executing_worker )
    {
        a_task_cb.m_executing_worker->post_task( a_task );
        return;
    }
    else
    {
        std::shared_ptr<abstract_worker> worker;
        worker = find_idle_worker();
        if( worker )
        {
            assign_work( worker, a_task );
            a_task_cb.m_executing_worker = worker;
            return;
        }
        else
        {
            // There are maybe no more workers. So we cache this task.
            a_task_cb.pending_tasks.push_back( a_task );
            return;
        }
    }
}

void thread_manager::schedule_immediately_task
    (
    std::shared_ptr<abstract_task> a_task,
    std::string const& a_module
    )
{
    s_thread_module_owner = a_module;
    auto_guard guard( [this]() { s_thread_module_owner.clear(); } );
    auto detail_module = framework_manager::get_instance().get_module_manager().get_module( a_module );
    if( detail_module )
    {
        detail_module->handle_task( a_task );
    }
    else
    {
        LogUtilError() << "No such module: " << a_module;
    }
    return;
}

void thread_manager::schedule_concurrently_task
    (
    std::shared_ptr<abstract_task> a_task
    )
{
    std::shared_ptr<abstract_worker> worker;
    worker = find_idle_worker();
    if( worker )
    {
        assign_work( worker, a_task );
    }
    else
    {
        //There is no worker to do our work current.
        m_work_need_assign.push_back( a_task );
    }
}

void thread_manager::schedule_handler_task
    (
    std::shared_ptr<abstract_task> a_task
    )
{
    std::string const& _module = a_task->get_target_module();
    auto detail_module = framework_manager::get_instance().get_module_manager().get_module( _module );
    auto handler = detail_module->get_task_handler();
    if( handler )
    {
        handler->handle( std::move( a_task ) );
    }
    else
    {
        LogUtilError() << "module " << _module << " does not have a task handler."
            " but it is module_type is handler_shchedule.";
        schedule_concurrently_task( std::move( a_task ) );
    }
}

std::shared_ptr<abstract_worker> thread_manager::make_worker()
{
    std::string worker_name{ "worker" };
    worker_name.append( std::to_string( m_next_worker_id++ ) );
    std::shared_ptr<thread_worker> thread_worker_;
    thread_worker_ = std::make_shared<thread_worker>();
    thread_worker_->set_worker_name( worker_name );
    return thread_worker_;
}

}

