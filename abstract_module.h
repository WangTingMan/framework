#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <shared_mutex>

#include "framework_export.h"

namespace framework
{

class abstract_task;
class framework_event;

class FRAMEWORK_EXPORT abstract_module
{

public:

    enum class module_type : uint8_t
    {
        concurrently_executing = 0x01, // all tasks related with this module can be concurrently executed
        sequence_executing = 0x02,     // all tasks related with this module only can be sequentially executed
        execute_task_when_post = 0x03, // all tasks related with this module will be execute when post immediately
        handler_shchedule = 0x04,      // all tasks related with this module will be scheduled by handler if a handler registered.
                                       // otherwise, will be sequentially executed.
    };

    enum class powering_status : uint8_t
    {
        power_off = 0x00,
        power_oning = 0x01,
        power_on = 0x02,
        power_offing = 0x03
    };

    //constexpr static std::string_view s_timer_module_name = "timer_module";

    constexpr static const char* s_timer_module_name = "timer_module";
    constexpr static const char* s_module_manager_name = "module_manager";
    constexpr static const char* s_task_runner_module_name = "task_runner_module_name";
    constexpr static const char* s_general_seq_task_runner_module = "general_seq_task_runner_module";

    virtual ~abstract_module(){}

    /**
     * initialize this module self.
     */
    virtual void initialize() = 0;

    /**
     * deinitialize this module self
     */
    virtual void deinitialize() = 0;

    /**
     * Handle a module task
     */
    virtual void handle_task( std::shared_ptr<abstract_task> a_task ) = 0;

    /**
     * Handle a module event.
     */
    virtual void handle_event( std::shared_ptr<framework_event> a_event ) = 0;

    std::string const& get_name()const
    {
        return m_module_name;
    }

    module_type const& get_module_type()const
    {
        return m_module_type;
    }

    powering_status const& get_power_status()const;

    static std::string to_string( powering_status const& a_status );

protected:

    void set_name( std::string a_module_name );

    void set_module_type( module_type a_type )
    {
        m_module_type = a_type;
    }

    void set_power_status( powering_status a_status );

private:

    // After the module created, should not change name and type. So no need to protect with mutex.
    std::string m_module_name;
    module_type m_module_type = module_type::concurrently_executing;

    mutable std::shared_mutex m_mutex;
    powering_status m_power_status = powering_status::power_on; // We treat a module do not need power on as default.
};

}

