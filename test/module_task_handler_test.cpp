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
 * Each module can have a task handler, which has an User-defined task schedule behavior.
 * For example, we want two or more sequence modules' tasks will be executed in same sequence.
 * And if two or more sequence modules have a same handler, then that same handler will
 * schedule all these tasks which belong to that modules.
 */
#include <iostream>
#include <random>
#include <thread>

#include "framework/abstract_module.h"
#include "framework/framework_manager.h"
#include "framework/log_util.h"
#include "framework/module_task_handler.h"
#include "framework/timer_module.h"

enum class module_with_handler_task_type : uint8_t
{
    invalid_task_type = 0,
    log_printing_type = 1
};

class module_with_handler_task : public framework::abstract_task
{

public:

    module_with_handler_task()
    {
    }

    module_with_handler_task_type type = module_with_handler_task_type::invalid_task_type;
};

class module_with_handler : public framework::abstract_module
{

public:

    module_with_handler(std::string a_module_name)
    {
        set_name( a_module_name );
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
        std::shared_ptr<module_with_handler_task> detail_task;
        detail_task = std::dynamic_pointer_cast<module_with_handler_task>( a_task );
        if( !detail_task )
        {
            return;
        }

        switch( detail_task->type )
        {
        case module_with_handler_task_type::log_printing_type:
            LogUtilInfo() << "Print a log1 to check executing.";
            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
            LogUtilInfo() << "Print a log2 to check executing.";
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
};

std::vector<std::string> module_task_handler_names;

std::vector<std::shared_ptr<framework::abstract_module>> generate_moudles()
{
    std::vector<std::shared_ptr<framework::abstract_module>> modules;

    auto handler = std::make_shared<framework::module_task_handler>();
    std::shared_ptr<module_with_handler> moudle_;
    for( int i = 0; i < 20; ++i )
    {
        std::string name{ "module_with_handler_" };
        name.append( std::to_string( i ) );
        module_task_handler_names.push_back( name );
        moudle_ = std::make_shared<module_with_handler>( name );
        moudle_->set_task_handler( handler );
        modules.push_back( std::move( moudle_ ) );
    }

    handler = std::make_shared<framework::module_task_handler>();
    for( int i = 50; i < 70; ++i )
    {
        std::string name{ "module_with_handler_" };
        name.append( std::to_string( i ) );
        module_task_handler_names.push_back( name );
        moudle_ = std::make_shared<module_with_handler>( name );
        moudle_->set_task_handler( handler );
        modules.push_back( std::move( moudle_ ) );
    }

    return modules;
}

int get_rand( int a_min, int a_max )
{
    return ( std::rand() % ( a_max - a_min + 1 ) ) + a_min;
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

    framework::framework_manager::get_instance().run( std::bind(&generate_moudles ), false );
    framework::framework_manager::get_instance().power_up();

    std::shared_ptr<module_with_handler_task> task;
    for( auto i = 0; i < 2000; ++i )
    {
        task = std::make_shared<module_with_handler_task>();
        srand( std::chrono::steady_clock::now().time_since_epoch().count() );
        int index = get_rand( 0, module_task_handler_names.size() - 1 );
        task->set_target_module( module_task_handler_names[index] );
        task->type = module_with_handler_task_type::log_printing_type;
        framework::framework_manager::get_instance().get_thread_manager().post_task( task );
    }

    std::this_thread::sleep_for( std::chrono::minutes( 5 ) );

    for( auto i = 0; i < 2000; ++i )
    {
        task = std::make_shared<module_with_handler_task>();
        srand( std::chrono::steady_clock::now().time_since_epoch().count() );
        int index = get_rand( 0, module_task_handler_names.size() - 1 );
        task->set_target_module( module_task_handler_names[index] );
        task->type = module_with_handler_task_type::log_printing_type;
        framework::framework_manager::get_instance().get_thread_manager().post_task( task );
    }


    std::this_thread::sleep_for( std::chrono::minutes( 50 ) );
    std::cout << "Hello World!\n";
}


