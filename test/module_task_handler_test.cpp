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

int main()
{
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


