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

#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <shared_mutex>

#include "framework_export.h"

namespace framework
{

class abstract_task;
class framework_event;
class module_task_handler;

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

    void set_task_handler( std::shared_ptr<module_task_handler> a_task_handler )
    {
        std::lock_guard locker( m_mutex );
        m_task_handler = std::move( a_task_handler );
        set_module_type( module_type::handler_shchedule );
    }

    std::shared_ptr<module_task_handler> get_task_handler()const
    {
        std::shared_lock locker( m_mutex );
        return m_task_handler;
    }

    /**
     * Try to get current executing thread id. A module may have many task to
     * execute, and these tasks may executed in same thread, for example: for
     * sequence_executing type, then all same moudle's task will be executed in
     * same thread one by one. So this API to get current thrad id. But sometime,
     * the framework do not schedule a thread for this module yet, then will return
     * a empty optional value.
     * Warning: this API only valid when type is sequence_executing or handler_shchedule.
     */
    std::optional<int> get_scheduled_thread_id()const;

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
    std::shared_ptr<module_task_handler> m_task_handler; // Not null if m_module_type equals handler_shchedule
};

}

