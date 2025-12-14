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

#include <future>
#include <iostream>
#include <thread>
#include "framework/framework_manager.h"
#include "framework/timer_module.h"
#include "framework/log_util.h"
#include "framework/utils.h"

std::promise<void> promis_;

void timer_handler_for_sometimes( uint32_t a_id, std::string a_name )
{
    LogUtilInfo( "timer: %s occurred.", a_name.c_str() );
}

void once_timer( uint32_t a_id, std::string a_name )
{
    LogUtilInfo() << "timer: " << a_name << " occurred.";
}

void usually_timer( uint32_t a_id, std::string a_name )
{
    static uint32_t occur_time = 0;
    occur_time++;
    LogUtilInfo() << "timer: " << a_name << " occurred.";
    if( occur_time > 30 )
    {
        auto timer_module_ = framework::framework_manager::get_instance()
            .get_module_manager().get_module< framework::timer_module >(
                framework::timer_module::s_timer_module_name );
        timer_module_->undregister_timer( a_id );
        LogUtilInfo() << "Cancel timer: " << a_id << ", name: " << a_name;
        promis_.set_value();
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

int main(int argc, char* argv[])
{
    if( argc > 0 )
    {
        set_log_location( argv[0] );
    }

    framework::framework_manager::get_instance().run( nullptr );
    framework::framework_manager::get_instance().power_up();
    auto timer_module_ = std::dynamic_pointer_cast< framework::timer_module>(
        framework::framework_manager::get_instance().get_module_manager().get_module(
            framework::timer_module::s_timer_module_name ) );
    framework::timer_control_block::timeout_callback time_cb;
    using cb_t = framework::timer_control_block::timeout_callback;

    time_cb = std::bind( &once_timer, std::placeholders::_1, std::placeholders::_2 );
    uint32_t timer1 = timer_module_->register_once_timer( time_cb,
        std::chrono::seconds( 2 ), "once_timer");

    uint32_t timer2 = timer_module_->register_timer(
        static_cast<cb_t>( std::bind( &timer_handler_for_sometimes, std::placeholders::_1, std::placeholders::_2 ) ),
        std::chrono::seconds( 1 ), "10_times_timer", 10 );

    uint32_t timer3 = timer_module_->register_timer(
        static_cast<cb_t>( std::bind( &usually_timer, std::placeholders::_1, std::placeholders::_2 ) ),
        std::chrono::seconds( 1 ), "usually_timer", 0 );

    std::future<void> fucture_ = promis_.get_future();
    fucture_.wait();

    std::cout << "Test done!\n";
}

