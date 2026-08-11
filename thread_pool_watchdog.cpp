#include "thread_pool_watchdog.h"
#include "log_util.h"
#include "internal/platform.h"

#include <algorithm>
#include <cstdlib>

namespace framework
{

thread_pool_watchdog::thread_pool_watchdog()
    : m_idle_since( std::chrono::steady_clock::now() )
{
}

thread_pool_watchdog::~thread_pool_watchdog()
{
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        m_stop = true;
    }
    m_condition.notify_one();
    if( m_thread.joinable() )
    {
        m_thread.join();
    }
}

void thread_pool_watchdog::default_timeout( monitored_task const& a_task )
{
    LogUtilFatal() << "thread id: " << a_task.thread_id << " may stuck.";
}

void thread_pool_watchdog::ensure_thread_started()
{
    if( !m_thread.joinable() )
    {
        m_thread = std::thread( &thread_pool_watchdog::run, this );
    }
}

void thread_pool_watchdog::task_started( uint64_t a_thread_id )
{
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        ensure_thread_started();
        m_tasks[a_thread_id] = { std::chrono::steady_clock::now(), false };
        m_state = state::monitoring;
    }
    m_condition.notify_one();
}

void thread_pool_watchdog::task_finished( uint64_t a_thread_id )
{
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        m_tasks.erase( a_thread_id );
        if( m_tasks.empty() )
        {
            m_idle_since = std::chrono::steady_clock::now();
            m_state = state::idle_observation;
        }
    }
    m_condition.notify_one();
}

void thread_pool_watchdog::set_idle_delay( std::chrono::milliseconds a_delay )
{
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        m_idle_delay = std::max( a_delay, std::chrono::milliseconds::zero() );
    }
    m_condition.notify_one();
}

void thread_pool_watchdog::set_task_timeout( std::chrono::milliseconds a_timeout )
{
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        m_task_timeout = std::max( a_timeout, std::chrono::milliseconds( 1 ) );
    }
    m_condition.notify_one();
}

void thread_pool_watchdog::set_timeout_callback( timeout_callback a_callback )
{
    std::lock_guard<std::mutex> locker( m_mutex );
    m_timeout_callback = std::move( a_callback );
}

thread_pool_watchdog::state thread_pool_watchdog::get_state()const
{
    std::lock_guard<std::mutex> locker( m_mutex );
    return m_state;
}

std::vector<thread_pool_watchdog::monitored_task> thread_pool_watchdog::monitored_tasks()const
{
    std::vector<monitored_task> result;
    std::lock_guard<std::mutex> locker( m_mutex );
    result.reserve( m_tasks.size() );
    for( auto const& task : m_tasks )
    {
        result.push_back( { task.first, task.second.started_at } );
    }
    return result;
}

void thread_pool_watchdog::run()
{
    framework::set_thread_name( "watch_dog" );
    std::unique_lock<std::mutex> locker( m_mutex );
    while( !m_stop )
    {
        if( m_tasks.empty() )
        {
            auto sleep_at = m_idle_since + m_idle_delay;
            if( m_state == state::idle_observation &&
                std::chrono::steady_clock::now() < sleep_at )
            {
                m_condition.wait_until( locker, sleep_at );
                continue;
            }

            m_state = state::sleeping;
            m_condition.wait( locker, [this]() { return m_stop || !m_tasks.empty(); } );
            continue;
        }

        m_state = state::monitoring;
        auto next_check = std::chrono::steady_clock::time_point::max();
        auto now = std::chrono::steady_clock::now();
        std::vector<monitored_task> timed_out;
        for( auto& task : m_tasks )
        {
            auto timeout_at = task.second.started_at + m_task_timeout;
            if( !task.second.timeout_reported && timeout_at <= now )
            {
                task.second.timeout_reported = true;
                timed_out.push_back( { task.first, task.second.started_at } );
            }
            else if( !task.second.timeout_reported )
            {
                next_check = std::min( next_check, timeout_at );
            }
        }

        auto callback = m_timeout_callback;
        if( !timed_out.empty() )
        {
            locker.unlock();
            for( auto const& task : timed_out )
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - task.started_at );
                LogUtilWarning() << "Thread pool watchdog detected a timed-out task. thread: "
                    << task.thread_id << ", elapsed: " << elapsed.count() << " ms.";
                if( callback )
                {
                    callback( task );
                }
                else
                {
                    default_timeout( task );
                }
            }
            locker.lock();
            continue;
        }

        if( next_check == std::chrono::steady_clock::time_point::max() )
        {
            m_condition.wait( locker );
        }
        else
        {
            m_condition.wait_until( locker, next_check );
        }
    }
}

}
