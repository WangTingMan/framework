#pragma once
#include "abstract_module.h"
#include "abstract_task.h"
#include "framework_export.h"

#include <list>
#include <memory>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <tuple>

namespace framework
{

class FRAMEWORK_EXPORT module_manager : public abstract_module
{

public:

    module_manager();

    void initialize()override;

    void deinitialize()override;

    /**
     * schedule the task. the task maybe a normal task, or event.
     */
    void schedule_task( std::shared_ptr<abstract_task> a_task );

    void handle_task( std::shared_ptr<abstract_task> a_task )override;

    void handle_event( std::shared_ptr<framework_event> a_event )override;

    void load_modules( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker );

    std::shared_ptr<abstract_module> get_module( std::string a_name )const;

    template<typename module_type>
    std::shared_ptr<module_type> get_module( std::string a_name )const
    {
        auto module_ = get_module( a_name );
        auto module_ret = std::dynamic_pointer_cast<module_type>( module_ );
        return module_ret;
    }

    void add_new_module( std::shared_ptr<framework::abstract_module> a_module );

    void remove_module( std::string a_name );

    void register_power_changed_callback( std::function<void( powering_status )> a_callback )
    {
        std::lock_guard<std::shared_mutex> locker( m_pro_mutex );
        m_power_changed_callback = a_callback;
    }

private:

    /**
     * return true indicate that the event need pass to all modules if need
     */
    bool handle_local_event( std::shared_ptr<framework_event> a_event );

    bool handle_power_on( std::shared_ptr<framework_event> const& a_event );

    bool handle_power_off( std::shared_ptr<framework_event> const& a_event );

    void handle_module_manager_task( std::shared_ptr<abstract_task> a_task );

    bool handle_module_power_changed( std::string a_module_name );

    std::tuple<size_t, size_t, size_t, size_t, size_t> get_module_status();

    std::shared_mutex m_pro_mutex;
    std::unordered_map<std::string, std::shared_ptr<abstract_module>> m_modules;
    std::function<void( powering_status )> m_power_changed_callback;
};

}

