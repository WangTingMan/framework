#include "module_manager.h"
#include "abstract_task.h"
#include "log_util.h"
#include "timer_module.h"
#include "task_runner_module.h"
#include "thread_manager.h"
#include "framework_manager.h"
#include "framework_event.h"
#include "executable_task.h"
#include "general_seq_task_runner_module.h"

namespace framework
{

module_manager::module_manager()
{
    set_name( s_module_manager_name );
}

void module_manager::initialize()
{
    framework_manager::get_instance().get_thread_manager()
        .register_module_type( get_module_type(), get_name() );

    for( auto& ele : m_modules )
    {
        framework_manager::get_instance().get_thread_manager()
            .register_module_type( ele.second->get_module_type(),
                ele.second->get_name() );
    }

    set_power_status( abstract_module::powering_status::power_off );
    for( auto& ele : m_modules )
    {
        ele.second->initialize();
    }
}

void module_manager::deinitialize()
{
    for( auto& ele : m_modules )
    {
        ele.second->deinitialize();
    }
}

void module_manager::schedule_task( std::shared_ptr<abstract_task> a_task )
{
    if( task_type::normal_type == a_task->get_task_type() )
    {
        handle_task( a_task );
    }
    else if( task_type::framework_event == a_task->get_task_type() )
    {
        std::shared_ptr<framework_event> event_;
        event_ = std::dynamic_pointer_cast< framework_event >( a_task );
        if( event_ )
        {
            handle_event( event_ );
        }
        else
        {
            LogUtilError() << "error task type!";
        }
    }
    else if( task_type::executable_task == a_task->get_task_type() )
    {
        std::shared_ptr<executable_task> tsk;
        tsk = std::dynamic_pointer_cast< executable_task >( a_task );
        tsk->run_task();
    }
    else
    {
        LogUtilError() << "unknown task type.";
    }
}

void module_manager::handle_task( std::shared_ptr<abstract_task> a_task )
{
    std::string const& _target_name = a_task->get_target_module();
    std::string const& _source_name = a_task->get_source_module();
    auto _module = get_module( _target_name );
    if( _module )
    {
        _module->handle_task( a_task );
    }
    else if( _target_name == get_name() )
    {
        handle_module_manager_task( a_task );
    }
    else if( _target_name.empty() )
    {
        for( auto& ele : m_modules )
        {
            if( _source_name != ele.second->get_name() )
            {
                ele.second->handle_task( a_task );
            }
        }
    }
    else
    {
        LogUtilError() << "Cannot find moudle: " << a_task->get_target_module();
    }
}

void module_manager::handle_event( std::shared_ptr<framework_event> a_event )
{
    std::string const& _target_name = a_event->get_target_module();
    std::string const& _source_name = a_event->get_source_module();
    if( _target_name == get_name() )
    {
        handle_local_event( a_event );
        return;
    }

    auto _module = get_module( _target_name );
    if( _module )
    {
        _module->handle_event( a_event );
    }
    else if( _target_name == get_name() )
    {
        LogUtilError() << "To handle this case.";
    }
    else if( _target_name.empty() )
    {
        bool pass_to_other_module = true;
        pass_to_other_module = handle_local_event( a_event );
        if( pass_to_other_module )
        {
            for( auto& ele : m_modules )
            {
                if( _source_name != ele.second->get_name() )
                {
                    ele.second->handle_event( a_event );
                }
            }
        }
    }
    else
    {
        LogUtilError() << "Unknown module: " << _target_name << ", from module " << _source_name;
    }
}

bool module_manager::handle_local_event( std::shared_ptr<framework_event> a_event )
{
    bool pass_all = true;
    switch( a_event->m_event_type )
    {
    case event_type::power_status_changed:
        pass_all = handle_module_power_changed( a_event->m_module_name );
        break;
    case event_type::power_on:
        pass_all = handle_power_on( a_event );
        break;
    case event_type::power_off:
        pass_all = handle_power_off( a_event );
        break;
    case event_type::derived_type:
        break;
    default:
        LogUtilWarning() << "event has been ignored: " << static_cast< uint16_t >( a_event->m_event_type );
        break;
    }
    return pass_all;
}

bool module_manager::handle_power_on( std::shared_ptr<framework_event> const& a_event )
{
    auto [power_on_cnt, power_off_cnt, power_oning_cnt, power_offing_cnt, total_cnt]
        = get_module_status();

    if( power_on_cnt == total_cnt )
    {
        LogUtilInfo() << "All modules have been powered on. So ignore power on task";
        return false;
    }

    if( power_on_cnt + power_oning_cnt == total_cnt )
    {
        LogUtilInfo() << "We are powering on. So ignore power on task";
        return false;
    }

    if( power_offing_cnt > 0 )
    {
        LogUtilError() << "We are powering off. So we cannot power on right now.";
        return false;
    }

    return true;
}

bool module_manager::handle_power_off( std::shared_ptr<framework_event> const& a_event )
{
    auto [power_on_cnt, power_off_cnt, power_oning_cnt, power_offing_cnt, total_cnt]
        = get_module_status();

    if( power_off_cnt == total_cnt )
    {
        LogUtilInfo() << "Already powered off. So ignore power off task";
        return false;
    }

    if( power_off_cnt + power_offing_cnt == total_cnt )
    {
        LogUtilInfo() << "We are powering off. So ignore this task";
        return false;
    }

    if( power_oning_cnt > 0 )
    {
        LogUtilError() << "We are powering on. So we cannot power off right now";
        return false;
    }

    return true;
}

bool module_manager::handle_module_power_changed( std::string a_module_name )
{
    auto [power_on_cnt, power_off_cnt, power_oning_cnt, power_offing_cnt, total_cnt]
        = get_module_status();

    LogUtilInfo() << a_module_name << " power status changed. "
        << power_on_cnt << " modules powred on. and " << power_off_cnt
        << " modules powered off.";

    powering_status pre_pwr_status = get_power_status();

    if( power_on_cnt == total_cnt )
    {
        set_power_status( abstract_module::powering_status::power_on );
    }
    else if( power_off_cnt == total_cnt )
    {
        set_power_status( abstract_module::powering_status::power_off );
    }

    powering_status now_pwr_status = get_power_status();
    if( now_pwr_status != pre_pwr_status )
    {
        LogUtilInfo() << "module manager's power status changed.";
        std::function<void( powering_status )> callback;

        std::shared_lock<std::shared_mutex> locker( m_pro_mutex );
        callback = m_power_changed_callback;
        locker.unlock();
        if( !callback )
        {
            LogUtilError() << "Not register m_power_changed_callback. How to notify power status change?";
            return true;
        }

        std::shared_ptr<executable_task> task;
        task = std::make_shared<executable_task>( [callback, now_pwr_status]()->bool
            {
                callback( now_pwr_status );
                return false;
            } );
        task->set_target_module( s_general_seq_task_runner_module );
        task->set_source_module( get_name() );
        framework_manager::get_instance().get_thread_manager().post_task( task );
    }

    return true;
}

std::tuple<size_t, size_t, size_t, size_t, size_t> module_manager::get_module_status()
{
    int power_on_cnt = 0;
    int power_off_cnt = 0;
    int power_oning_cnt = 0;
    int power_offing_cnt = 0;

    std::lock_guard<std::shared_mutex> locker( m_pro_mutex );
    for( auto& ele : m_modules )
    {
        auto& status = ele.second->get_power_status();
        switch( status )
        {
        case powering_status::power_on:
            power_on_cnt++;
            break;
        case powering_status::power_off:
            power_off_cnt++;
            break;
        case powering_status::power_oning:
            power_oning_cnt++;
            break;
        case powering_status::power_offing:
            power_offing_cnt++;
            break;
        default:
            break;
        }
    }

    if( power_offing_cnt > 0 && power_oning_cnt > 0 )
    {
        LogUtilError() << "Some module powering on and some module powering off?";
    }

    return { power_on_cnt, power_off_cnt, power_oning_cnt, power_offing_cnt, m_modules.size() };
}

void module_manager::load_modules( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker )
{
    std::vector<std::shared_ptr<abstract_module>> _modules;
    if( a_module_maker )
    {
        _modules = a_module_maker();
    }

    _modules.push_back( std::make_shared<timer_module>() );
    _modules.push_back( std::make_shared<task_runner_module>() );
    _modules.push_back( std::make_shared<general_seq_task_runner_module>() );

    std::lock_guard<std::shared_mutex> locker( m_pro_mutex );

    for( auto& ele : _modules )
    {
        if( !m_modules[ele->get_name()] )
        {
            m_modules[ele->get_name()] = ele;
            LogUtilInfo() << "Loaded module: " << ele->get_name();
            if( ele->get_name().empty() )
            {
                LogUtilError() << "Must assign a name for the module!";
            }
        }
        else
        {
            LogUtilWarning() << "Already load module: " << ele->get_name();
        }
    }
}

std::shared_ptr<abstract_module> module_manager::get_module( std::string a_name )const
{
    auto it = m_modules.find( a_name );
    if( it != m_modules.end() )
    {
        return it->second;
    }
    return nullptr;
}

void module_manager::add_new_module( std::shared_ptr<framework::abstract_module> a_module )
{
    if( !a_module )
    {
        LogUtilError() << "To add empty module??";
        return;
    }

    std::lock_guard<std::shared_mutex> locker( m_pro_mutex );
    if( m_modules.find( a_module->get_name() ) == m_modules.end() )
    {
        m_modules[a_module->get_name()] = a_module;
        framework_manager::get_instance().get_thread_manager()
            .register_module_type( a_module->get_module_type(),
                a_module->get_name() );
        a_module->initialize();
        powering_status  current_power_status = get_power_status();
        if( current_power_status == abstract_module::powering_status::power_on ||
            current_power_status == abstract_module::powering_status::power_oning )
        {
            std::shared_ptr<framework_event> event_ = std::make_shared<framework_event>();
            event_->m_event_type = event_type::power_on;
            a_module->handle_event( event_ );
        }
        else if( current_power_status == abstract_module::powering_status::power_off ||
            current_power_status == abstract_module::powering_status::power_offing )
        {
            std::shared_ptr<framework_event> event_ = std::make_shared<framework_event>();
            event_->m_event_type = event_type::power_off;
            a_module->handle_event( event_ );
        }
        else
        {
            LogUtilError() << "unknown power status.";
        }
    }
    else
    {
        LogUtilError() << "Already has module: " << a_module->get_name();
    }
}

void module_manager::handle_module_manager_task( std::shared_ptr<abstract_task> a_task )
{

}

}
