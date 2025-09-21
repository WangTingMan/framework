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

#ifdef PRINT_LOG
#define LOGGER LogUtilInfo
#else
#define LOGGER LogUtilIgnore
#endif

enum class module_with_handler_task_type : uint8_t
{
    invalid_task_type = 0,
    random_access_dynamic_memory = 1,
    allocate_memory = 2
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
        allocate_new_memory();
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
        case module_with_handler_task_type::random_access_dynamic_memory:
            random_access_memory();
            break;
        case module_with_handler_task_type::allocate_memory:
            allocate_new_memory();
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

    void allocate_new_memory()
    {
        if( m_dynamic_memory )
        {
            LOGGER() << "free memory: " << std::format( "{:16p}", (void*)m_dynamic_memory );
            delete[] m_dynamic_memory;
            m_dynamic_memory = nullptr;
        }

        m_memmory_size = get_rand( 1024, 4096 );
        m_dynamic_memory = new char[m_memmory_size];
        LOGGER() << "allocate memory: " << std::format( "{:16p}", (void*)m_dynamic_memory ) << ", size: " << m_memmory_size;
    }

    void random_access_memory()
    {
        uint32_t pos = get_rand( 0, m_memmory_size - 1 );
        m_dynamic_memory[pos] = 'Y';
        LOGGER() << "access memory: " << std::format( "{:16p}", (void*)m_dynamic_memory ) << ", pos = " << pos;
    }

    char* m_dynamic_memory = nullptr;
    uint32_t m_memmory_size = 0;
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

void generate_task()
{
    std::shared_ptr<sequence_module_example_task> task;
    for( auto i = 0; i < 20000; ++i )
    {
        task = std::make_shared<sequence_module_example_task>();
        srand( std::chrono::steady_clock::now().time_since_epoch().count() );
        uint32_t index = get_rand( 0, module_task_handler_names.size() - 1 );
        task->set_target_module( module_task_handler_names[index] );
        uint32_t rand_value = get_rand( 0, 20 );
        if( rand_value > 10 )
        {
            task->type = module_with_handler_task_type::allocate_memory;
        }
        else
        {
            task->type = module_with_handler_task_type::random_access_dynamic_memory;
        }
        framework::framework_manager::get_instance().get_thread_manager().post_task( task );
    }
}

void set_log_location( char const* a_module_path )
{
    const char* file = a_module_path;
    const char* file_name = nullptr;

    while( file && *( file++ ) != '\0' )
    {
        if( '/' == *file || '\\' == *file )
        {
            file_name = file + 1;
        }
    }

    if( file_name == nullptr )
    {
        return;
    }

    std::string log_file_name;
    log_file_name.assign( file_name );

    std::string log_path( a_module_path, file_name );

    framework::set_default_log_location( log_path, log_file_name );
}

int main( int argc, char* argv[] )
{
    if( argc > 0 )
    {
        set_log_location( argv[0] );
    }

    framework::framework_manager::get_instance().run( std::bind( &generate_moudles ), false );
    framework::framework_manager::get_instance().power_up();

    for( int i = 0; i < 10; ++i )
    {
        std::thread th( &generate_task );
        th.detach();
    }

    std::this_thread::sleep_for( std::chrono::minutes( 10 ) );
    return 0;
}

