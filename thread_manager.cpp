#include "thread_manager.h"
#include "framework_manager.h"
#include "timer_module.h"
#include "thread_worker.h"
#include "log_util.h"
#include "executable_task.h"

namespace framework
{

void thread_manager::run( bool a_occupy_current_thread )
{
    std::shared_ptr<thread_worker> current_thread_worker;
    if( a_occupy_current_thread )
    {
        current_thread_worker = std::make_shared<thread_worker>();
    }

    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    /**
     * If there is no worker to do work, then recruit some one.
     */
    if( m_idle_worker.empty() )
    {
        m_idle_worker.emplace_back( std::make_shared<thread_worker>() );
        m_idle_worker.emplace_back( std::make_shared<thread_worker>() );
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
    std::lock_guard<std::recursive_mutex> locker( m_mutex );

    std::string const& _module = a_task->get_target_module();
    if( _module.empty() )
    {
        if( a_task->get_task_type() == task_type::framework_event )
        {
            std::vector<std::shared_ptr<abstract_task>> tasks;
            for( auto& ele : m_module_types )
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

    if( !_module.empty() && !m_module_types.contains( _module ) )
    {
        LogUtilError() << "Such module has not registered: " << _module;
    }

    module_task_cb& cb = m_module_types[_module];
    if( cb.module_type_value == abstract_module::module_type::sequence_executing )
    {
        if( cb.m_executing_worker )
        {
            cb.m_executing_worker->post_task( a_task );
            return;
        }
        else
        {
            std::shared_ptr<abstract_worker> worker;
            worker = find_idle_worker();
            if( worker )
            {
                assign_work( worker, a_task );
                cb.m_executing_worker = worker;
                return;
            }
            else
            {
                // There are maybe no more workers. So we cache this task.
                cb.pending_tasks.push_back( a_task );
                return;
            }
        }
    }

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
    for( auto it = m_module_types.begin(); it != m_module_types.end(); ++it )
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
    if( !m_module_types.contains( a_module_name ) )
    {
        m_module_types[a_module_name] = cb;
    }
    else
    {
        LogUtilInfo() << "Already has " << a_module_name << ", change module tye.";
        m_module_types[a_module_name].module_type_value = a_type;
    }
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

    for( auto it = m_module_types.begin(); it != m_module_types.end(); ++it )
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
        std::shared_ptr<abstract_worker> worker = std::make_shared<thread_worker>();
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

}

