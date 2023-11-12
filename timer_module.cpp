#include "timer_module.h"
#include "abstract_task.h"
#include "executable_task.h"
#include "log_util.h"
#include "framework_manager.h"
#include "framework_event.h"

#include <chrono>
#include <limits>

#ifdef DEBUG_TIMER_MODULE
#define LogTimerDebug LogUtilInfo
#else
#define LogTimerDebug LogUtilIgnore
#endif

namespace framework
{

enum class timer_module_task_type : uint8_t
{
    invalid_task_type = 0,
    timer_schedule_task = 1
};

class timer_module_timer_task : public abstract_task
{

public:

    timer_module_timer_task() : schedule_duration(0)
    {}

    timer_module_task_type type = timer_module_task_type::invalid_task_type;
    std::chrono::milliseconds schedule_duration;
};

timer_module::timer_module()
{
    set_name( s_timer_module_name );
    set_module_type( abstract_module::module_type::concurrently_executing );
}

int64_t timer_module::get_system_booting_time()
{
    std::chrono::milliseconds sysUpTime =
        std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::high_resolution_clock::now() -
            std::chrono::high_resolution_clock::time_point() );
    return sysUpTime.count();
}

std::string timer_module::to_booting_time_stamp( int64_t a_booting_time )
{
    int64_t up_miliseconds = a_booting_time;
    std::chrono::milliseconds sys_up_time = std::chrono::milliseconds( up_miliseconds );
    int up_seconds = static_cast< int >(
        std::chrono::duration_cast< std::chrono::seconds >( sys_up_time ).count() );
    up_miliseconds -= up_seconds * 1000;
    int up_minutes = up_seconds / 60;
    up_seconds -= up_minutes * 60;
    int up_hours = up_minutes / 60;
    up_minutes -= up_hours * 60;

    char buffer[100];
    snprintf( buffer, sizeof buffer,
        "[%02d:%02d:%02d.%03d]", up_hours,
        up_minutes, up_seconds, static_cast<int>( up_miliseconds ) );

    return buffer;
}

void timer_module::initialize()
{
    set_power_status( abstract_module::powering_status::power_on );
}

void timer_module::deinitialize()
{
    set_power_status( abstract_module::powering_status::power_off );
}

void timer_module::handle_task( std::shared_ptr<abstract_task> a_task )
{
    std::shared_ptr<timer_module_timer_task> task;
    task = std::dynamic_pointer_cast<timer_module_timer_task>( a_task );
    if( !task )
    {
        return;
    }

    if( task->type != timer_module_task_type::timer_schedule_task )
    {
        return;
    }

    std::unique_lock<std::mutex> locker( m_condition_mutex );
    if( task->schedule_duration > std::chrono::milliseconds( 0 ) )
    {
        m_condition_waiting = true;
        m_condition.wait_for( locker, task->schedule_duration );
    }
    m_condition_waiting = false;
    locker.unlock();

    handle_timer_expired();
}

void timer_module::handle_event( std::shared_ptr<framework_event> a_event )
{
    switch( a_event->m_event_type )
    {
    case event_type::power_on:
        set_power_status( abstract_module::powering_status::power_on );
        break;
    case event_type::power_off:
        // Other module needs to cancel the registered timer.
        set_power_status( abstract_module::powering_status::power_off );
        break;
    case event_type::power_status_changed:
        break;
    case event_type::derived_type:
        break;
    default:
        LogUtilError() << "event type ignored: " << static_cast< uint16_t >( a_event->m_event_type );
        break;
    }
}

uint32_t timer_module::register_timer
    (
    timer_control_block::timeout_callback a_expire_callback,
    std::chrono::milliseconds a_interval,
    uint32_t a_trigger_times,
    std::string a_handle_module
    )
{
    return register_timer( a_expire_callback, a_interval, "", a_trigger_times, a_handle_module );
}

uint32_t timer_module::register_timer
    (
    timer_control_block::timeout_callback a_expire_callback,
    std::chrono::milliseconds a_interval,
    std::string a_timer_name,
    uint32_t a_trigger_times,
    std::string a_handle_module
    )
{
    std::shared_ptr<timer_control_block> timer = std::make_shared<timer_control_block>();
    timer->set_timeout_callback( a_expire_callback );
    timer->set_interval( static_cast< uint32_t >( a_interval.count() ) );
    timer->set_trigger_times( a_trigger_times );
    timer->set_timer_id( m_timer_count.fetch_add( 1 ) );
    timer->set_timer_name( a_timer_name );
    timer->set_handle_module( a_handle_module );
    timer->start_schedule();

    LogTimerDebug() << "Create timer " << a_timer_name << " done. First trigger is " << a_interval
        << " later. On timepoint: " << to_booting_time_stamp( timer->get_time_to_execute() );
    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    uint64_t nextFire = m_timers.empty() ? std::numeric_limits<uint64_t>::max() :
        m_timers.front()->get_time_to_execute();
    m_timers.push_front( timer );
    m_timers.sort( []( std::shared_ptr<timer_control_block> const& a_left,
        std::shared_ptr<timer_control_block> const& a_right )->bool
            {
                return a_left->get_time_to_execute() < a_right->get_time_to_execute();
            } );

    auto front_time_to_execute = m_timers.front()->get_time_to_execute();
    locker.unlock();

    if( front_time_to_execute < nextFire )
    {
        make_schedule_task_if_need( front_time_to_execute );
    }

    return timer->get_timer_id();
}

void timer_module::reset_timer
    (
    uint32_t a_id,
    std::chrono::milliseconds a_interval
    )
{
    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    for( auto it = m_timers.begin(); it != m_timers.end(); ++it )
    {
        if( ( *it )->get_timer_id() == a_id )
        {
            ( *it )->set_interval( a_interval.count() );
            return;
        }
    }
}

void timer_module::undregister_timer( uint32_t a_timer_id )
{
    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    for( auto it = m_timers.begin(); it != m_timers.end(); ++it )
    {
        if( ( *it )->get_timer_id() == a_timer_id )
        {
            m_timers.erase( it );
            return;
        }
    }
}

void timer_module::handle_timer_expired()
{
    std::unique_lock<std::recursive_mutex> locker( m_mutex );
    for( auto it = m_timers.begin(); it != m_timers.end(); ++it )
    {
        std::shared_ptr<timer_control_block>& _timer = *it;
        int64_t curTime = get_system_booting_time();
        int64_t executeTime = _timer->get_time_to_execute();
        int64_t diff = curTime - executeTime;
        if( diff < -10 )
        {
            // Seem like I'm wake up early
            break;
        }

        if( diff > 100 )
        {
            LogUtilWarning() << "timer need to be fire ealier. timer: " << _timer->get_timer_name()
                << ", diff = " << diff << ", curtime: " << to_booting_time_stamp( curTime ) << ", execute time: "
                << to_booting_time_stamp( executeTime );
        }

        int interval = _timer->get_interval();
        std::shared_ptr<executable_task> task;
        auto fun = _timer->get_timeout_callback();
        uint32_t timer_id = _timer->get_timer_id();
        std::string timer_name = _timer->get_timer_name();
        uint32_t remain_trigger_times = _timer->get_remain_trigger_timers();

        LogTimerDebug() << "timer: " << _timer->get_timer_name() << " remains " << remain_trigger_times
            << ", current execute time: " << to_booting_time_stamp( executeTime ) << ", current time: " << to_booting_time_stamp( curTime );

        task = std::make_shared<executable_task>( [fun, timer_id, timer_name, executeTime, interval, remain_trigger_times]()
                {
                    int64_t curTime = get_system_booting_time();
                    if( curTime - executeTime > 300 && remain_trigger_times > 0 )
                    {
                        /**
                         * If the there are many task in thread pool need to execute, then the repeat timer
                         * may execute many times at the same time. eg. if we have a debug break point here,
                         * then other thread is running and will run the timer so this case may be happen.
                         * So we need add a check here to avoid that. That is, for instance we have registered
                         * a repeating timer with interval is 1 seconds. And the add a debug break pointer here,
                         * then the timer thread will add 1, 2, 3, 4, 5, 6, 7 second timer event in thread pool.
                         * But all these timer event will execute at 7 second. So that unexpected case happen. We
                         * need avoid this. Here checked time is 300 milliseconds.
                         */
                        return false;
                    }

                    LogTimerDebug() << "trigger timer: " << timer_name << ", remain " << remain_trigger_times;
                    fun( timer_id, timer_name );
                    return false;
                } );
        std::string handle_module = _timer->get_handle_module();
        if( !handle_module.empty() )
        {
            // We will post this time out callback to the target module
            task->set_target_module( handle_module );
        }
        else
        {
            // We will find a thread to execute the time out callback. In this case
            // the user must be careful about the thread safe problem.
            task->set_target_module( s_task_runner_module_name );
        }

        task->set_source_module( get_name() );
        framework_manager::get_instance().get_thread_manager().post_task( task );

        _timer->timer_triggered();
        remain_trigger_times = _timer->get_remain_trigger_timers();
        LogTimerDebug() << "Now, timer: " << _timer->get_timer_name() << " remains " << remain_trigger_times;
    }

    if( m_timers.empty() )
    {
        return;
    }

    m_timers.sort( []( std::shared_ptr<timer_control_block> const& a_left,
                       std::shared_ptr<timer_control_block> const& a_right )->bool
                    {
                        return a_left->get_time_to_execute() < a_right->get_time_to_execute();
                    } );

    auto front_time_to_execute = m_timers.front()->get_time_to_execute();

    auto diff = front_time_to_execute - get_system_booting_time();
    if( diff < 0 )
    {
        /**
         * If the front time to execute is ealier that current time, then we just add 10 ms
         * to wait.
         */
        front_time_to_execute = get_system_booting_time() + 10;
    }

    make_schedule_task_if_need( front_time_to_execute );
}

void timer_module::make_schedule_task_if_need( int64_t a_front_time_to_execute )
{
    std::unique_lock<std::mutex> locker( m_condition_mutex );
    if( m_condition_waiting &&
        m_weak_up_time > a_front_time_to_execute )
    {
        LogTimerDebug() << "notify timer waiter.";
        m_condition.notify_all();
        return;
    }

    m_weak_up_time = a_front_time_to_execute;
    locker.unlock();

    std::shared_ptr<timer_module_timer_task> task;
    task = std::make_shared<timer_module_timer_task>();
    task->set_source_module( get_name() );
    task->set_target_module( get_name() );
    task->set_position( source_here );
    task->type = timer_module_task_type::timer_schedule_task;
    a_front_time_to_execute -= get_system_booting_time();
    LogTimerDebug() << "wait until " << to_booting_time_stamp( m_weak_up_time );
    task->schedule_duration = std::chrono::milliseconds( a_front_time_to_execute );

    framework_manager::get_instance().get_thread_manager().post_task( task );
}

}

