#include "abstract_module.h"
#include "abstract_task.h"
#include "framework_event.h"
#include "framework_manager.h"
#include "module_task_handler.h"

#include "log_util.h"

namespace framework
{

abstract_module::powering_status const& abstract_module::get_power_status()const
{
    std::shared_lock<std::shared_mutex> locker(m_mutex);
    return m_power_status;
}

std::string abstract_module::to_string( powering_status const& a_status )
{
    switch( a_status )
    {
    case powering_status::power_off:
        return "power off";
    case powering_status::power_offing:
        return "power offing";
    case powering_status::power_on:
        return "power on";
    case powering_status::power_oning:
        return "power oning";
    default:
        break;
    }
    return "";
}

void abstract_module::set_name( std::string a_module_name )
{
    m_module_name = a_module_name;
}

void abstract_module::set_power_status( powering_status a_status )
{
    bool changed = false;
    std::unique_lock<std::shared_mutex> locker( m_mutex );
    changed = m_power_status != a_status;
    m_power_status = a_status;
    if( changed )
    {
        LogUtilInfo() << "module " << get_name() << " power status changed to: " << to_string( m_power_status );
        locker.unlock();

        std::shared_ptr<framework_event> event_;
        event_ = std::make_shared<framework_event>();
        event_->m_module_name = m_module_name;
        event_->m_event_type = event_type::power_status_changed;
        event_->set_source_module( m_module_name );
        framework_manager::get_instance().get_thread_manager().post_task( event_ );
    }
}

std::optional<int> abstract_module::get_scheduled_thread_id()const
{
    switch( get_module_type() )
    {
    case module_type::concurrently_executing:
        [[fallthough]]
    case module_type::execute_task_when_post:
        return std::nullopt;
    case module_type::handler_shchedule:
        {
            std::shared_ptr<module_task_handler> handler = get_task_handler();
            if( handler )
            {
                return handler->get_current_executing_thread_id();
            }
        }
        return std::nullopt;
    case module_type::sequence_executing:
        {
            std::string const& name = get_name();
            uint64_t id = 0;
            id = framework_manager::get_instance().get_thread_manager().get_scheduled_thread_id(name);
            return static_cast<int>( id );
        }
        break;
    default:
        break;
    }

    return std::nullopt;
}

}

