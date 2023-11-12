#include "framework_manager.h"
#include "framework_event.h"
#include "timer_module.h"
#include "log_util.h"

#include <mutex>

namespace framework
{

framework_manager& framework_manager::get_instance()
{
    static framework_manager instance;
    return instance;
}

bool  framework_manager::is_running()const
{
    std::shared_lock<std::shared_mutex> locker( m_mutex );
    return m_is_running;
}

void framework_manager::run
    (
    std::function<std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker,
    bool a_occupy_current_thread
    )
{
    std::lock_guard<std::shared_mutex> locker( m_mutex );
    if( m_is_running )
    {
        return;
    }
    init( std::move( a_module_maker ) );
    m_is_running = true;
    m_thread_manager.run( a_occupy_current_thread );
}

void framework_manager::power_up()
{
    std::shared_ptr<framework_event> event_ = std::make_shared<framework_event>();
    event_->m_event_type = event_type::power_on;
    m_thread_manager.post_task( event_ );
}

void framework_manager::init( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker )
{
    m_module_manager.load_modules( std::move( a_module_maker ) );
    m_module_manager.initialize();
}

}

