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
#include "thread_worker.h"
#include "log_util.h"
#include "thread_manager.h"
#include "framework_manager.h"
#include "internal/platform.h"
#include "executable_task.h"
#include "abstract_module.h"

#include <iterator>

constexpr std::chrono::seconds s_idle_max_long_time( 10 );

namespace framework
{

thread_worker::thread_worker()
{
    m_last_executing_time = std::chrono::steady_clock::now();
    m_is_running = true;
}

thread_worker::~thread_worker()
{
    if( m_thread.joinable() )
    {
        m_thread.detach();
    }
}

void thread_worker::run
    (
    std::shared_ptr<abstract_worker> a_current,
    bool a_accupy_cureent_thread
    )
{
    if( a_accupy_cureent_thread )
    {
        run_impl( a_current );
    }
    else
    {
        if( !m_thread.joinable() )
        {
            m_thread = std::thread( &thread_worker::run_impl, this, a_current );
        }
        else
        {
            LogUtilWarning() << "Current thread is running?";
        }
    }
}

void thread_worker::post_task( std::shared_ptr<abstract_task> a_task )
{
    std::string const& debug_info = a_task->get_debug_info();
    if( !debug_info.empty() )
    {
        LogUtilInfo() << "post task. debug info: " << debug_info;
    }
    std::unique_lock<std::mutex> locker( m_mutex );
    m_tasks.emplace_back( std::move( a_task ) );
    locker.unlock();

    m_condition_variable.notify_all();
}

void thread_worker::post_task( std::vector<std::shared_ptr<abstract_task>> a_tasks )
{
    using iter_t = std::vector<std::shared_ptr<abstract_task>>::iterator;
    std::unique_lock<std::mutex> locker( m_mutex );
    m_tasks.insert( m_tasks.end(),
        std::move_iterator<iter_t>( a_tasks.begin() ),
        std::move_iterator<iter_t>( a_tasks.end() )
        );

    locker.unlock();

    m_condition_variable.notify_all();
}

bool thread_worker::is_idle_for_long_time()
{
    bool idle_long_time = false;
    auto now_ = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> locker( m_mutex );
    auto duration_ = now_ - m_last_executing_time;
    if( duration_ > s_idle_max_long_time )
    {
        idle_long_time = true;
    }

    return idle_long_time;
}

void thread_worker::exit_later()
{
    auto fun = []()
    {
        return false;
    };

    m_is_running.exchange( false );

    auto tsk = std::make_shared<executable_task>( fun );
    tsk->set_position( source_here );
    post_task( tsk );
}

uint64_t thread_worker::work_thread_id()
{
    return m_thread_id;
}

void thread_worker::run_impl( std::shared_ptr<abstract_worker> a_current )
{
    LogUtilDebug() << "thread work started.";
    m_thread_id = framework::get_current_thread_id();

    std::vector<std::shared_ptr<abstract_task>> tasks;
    std::unique_lock<std::mutex> locker( m_mutex, std::defer_lock );
    bool ret = false;

    framework::set_thread_name( "worker" );

    while( true )
    {
        if( !m_is_running )
        {
            break;
        }

        locker.lock();
        if( m_tasks.empty() )
        {
            locker.unlock();
            framework_manager::get_instance().get_thread_manager().push_idle_worker( a_current );
            locker.lock();
            m_condition_variable.wait( locker, [this]()
                {
                    return !m_tasks.empty();
                }
                );
        }
        tasks = std::move( m_tasks );
        locker.unlock();

        for( auto it = tasks.begin(); it != tasks.end(); ++it )
        {
            auto& the_task = ( *it );
            thread_manager::set_current_thread_module_owner( the_task->get_target_module() );
            auto_guard guard( []() { thread_manager::set_current_thread_module_owner( "" ); } );
            bool exit = handle_task( the_task );

            if( exit || (!m_is_running) )
            {
                framework_manager::get_instance().get_thread_manager().remove_worker( a_current );
                std::vector<std::shared_ptr<abstract_task>> unhandled_task;
                unhandled_task.assign( std::next( it ), tasks.end() );
                if( !unhandled_task.empty() )
                {
                    framework_manager::get_instance().get_instance()
                        .get_thread_manager().post_task( unhandled_task );
                }
                break;
            }
            m_last_executing_time = std::chrono::steady_clock::now();
        }
    }

    LogUtilDebug() << "thread work ended.";
}

bool thread_worker::handle_task( std::shared_ptr<abstract_task> const& a_task )
{
    std::string const& debug_info = a_task->get_debug_info();
    if( !debug_info.empty() )
    {
        LogUtilInfo() << "handle task. task debug info: " << debug_info;
    }

    bool handled = false;
    bool ret = false;
    if( a_task->get_target_module() == abstract_module::s_task_runner_module_name ||
        a_task->get_target_module().empty() )
    {
        auto runnable_task = std::dynamic_pointer_cast< executable_task >( a_task );
        if( runnable_task )
        {
            ret = runnable_task->run_task();
            if( ret )
            {
                auto postion_ = runnable_task->get_position();
                LogUtilInfo() << "From " << runnable_task->get_source_module()
                    << " to : " << runnable_task->get_target_module()
                    << ". Require quit current thread. postion: " << postion_.to_string();
            }
            handled = true;
        }
    }

    if( !handled )
    {
        framework_manager::get_instance().get_module_manager().schedule_task( a_task );
    }

    return ret;
}

}

