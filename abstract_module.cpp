#include "abstract_module.h"
#include "abstract_task.h"
#include "framework_event.h"
#include "framework_manager.h"

#include "log_util.h"

namespace framework
{

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

}

