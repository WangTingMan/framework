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

/**
 * Sequence modules' task scheduling press test.
 * If this process does not crash in 10 minutes, then
 * this testing pass.
 */

#include <format>
#include <iostream>
#include <random>
#include <thread>

#include "framework/abstract_module.h"
#include "framework/framework_manager.h"
#include "framework/log_util.h"
#include "framework/module_task_handler.h"
#include "framework/timer_module.h"

//#define PRINT_LOG

#ifdef PRINT_LOG
#define LOGGER LogUtilInfo
#else
#define LOGGER LogUtilIgnore
#endif

enum class module_with_handler_task_type : uint8_t
{
    invalid_task_type = 0,
    add_value_and_make_string_type = 1,
    check_value_and_string_type = 2
};

class sequence_module_example_task : public framework::abstract_task
{

public:

    sequence_module_example_task()
    {
    }

    module_with_handler_task_type type = module_with_handler_task_type::invalid_task_type;
};

uint32_t  get_rand( uint32_t  a_min, uint32_t  a_max )
{
    return ( std::rand() % ( a_max - a_min + 1 ) ) + a_min;
}

class sequence_module_example : public framework::abstract_module
{

public:

    sequence_module_example( std::string a_module_name )
    {
        set_name( a_module_name );
        set_module_type( framework::abstract_module::module_type::sequence_executing );
        m_string_prefix.assign( "sequence_module_task_test2, to test to executing: " );
        add_value_and_make_string();
    }

    /**
     * initialize this module self.
     */
    void initialize()
    {
        set_power_status( abstract_module::powering_status::power_on );
    }

    /**
     * deinitialize this module self
     */
    void deinitialize()
    {
        set_power_status( abstract_module::powering_status::power_off );
    }

    /**
     * Handle a module task
     */
    void handle_task( std::shared_ptr<framework::abstract_task> a_task )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( get_rand( 0, 10 ) ) );
        std::shared_ptr<sequence_module_example_task> detail_task;
        detail_task = std::dynamic_pointer_cast<sequence_module_example_task>( a_task );
        if( !detail_task )
        {
            return;
        }

        switch( detail_task->type )
        {
        case module_with_handler_task_type::add_value_and_make_string_type:
            add_value_and_make_string();
            break;
        case module_with_handler_task_type::check_value_and_string_type:
            check_value_and_string();
            break;
        default:
            break;
        }
    }

    /**
     * Handle a module event.
     */
    void handle_event( std::shared_ptr<framework::framework_event> a_event )
    {

    }

private:

    void add_value_and_make_string()
    {
        uint32_t add_value = get_rand( 5, 15 );
        uint32_t previous_value = m_count;
        LOGGER() << "previous value = " << m_count << ", add value = " << add_value;
        m_count += add_value;
        std::string str{ m_string_prefix };
        str.append( std::to_string( m_count ) );
        m_count_string = str;
        LOGGER() << "now we have string: " << str;
    }

    void check_value_and_string()
    {
        LOGGER() << "To check value.";
        std::string str{ m_string_prefix };
        str.append( std::to_string( m_count ) );
        if( str != m_count_string )
        {
            LogUtilFatal() << "Not equal!";
        }
        else
        {
            LOGGER() << "value = " << m_count << " and string: " << m_count_string;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    uint32_t m_count = 0;
    std::string m_count_string;
    std::string m_string_prefix;
};

std::vector<std::string> module_task_handler_names;

std::vector<std::shared_ptr<framework::abstract_module>> generate_moudles()
{
    std::vector<std::shared_ptr<framework::abstract_module>> modules;

    std::shared_ptr<sequence_module_example> moudle_;
    for( int i = 0; i < 20; ++i )
    {
        std::string name{ "sequence_module_example_" };
        name.append( std::to_string( i ) );
        moudle_ = std::make_shared<sequence_module_example>( name );
        modules.push_back( std::move( moudle_ ) );
        module_task_handler_names.push_back( name );
    }

    return modules;
}

void generate_task(int task_count = 20000)
{
    std::shared_ptr<sequence_module_example_task> task;
    for( auto i = 0; i < task_count; ++i )
    {
        task = std::make_shared<sequence_module_example_task>();
        srand( std::chrono::steady_clock::now().time_since_epoch().count() );
        uint32_t index = get_rand( 0, module_task_handler_names.size() - 1 );
        task->set_target_module( module_task_handler_names[index] );
        uint32_t rand_value = get_rand( 0, 20 );
        if( rand_value > 10 )
        {
            task->type = module_with_handler_task_type::add_value_and_make_string_type;
        }
        else
        {
            task->type = module_with_handler_task_type::check_value_and_string_type;
        }
        framework::framework_manager::get_instance().get_thread_manager().post_task( task );
    }
}

int main()
{
    framework::framework_manager::get_instance().run( std::bind( &generate_moudles ), false );
    framework::framework_manager::get_instance().power_up();

    for( int i = 0; i < 10; ++i )
    {
        std::thread th( &generate_task, 20000 );
        th.detach();
    }

    std::this_thread::sleep_for( std::chrono::minutes( 10 ) );

    generate_task( 2 );

    std::this_thread::sleep_for( std::chrono::minutes( 2 ) );
    generate_task( 1 );
    std::this_thread::sleep_for( std::chrono::minutes( 1 ) );
    return 0;
}

