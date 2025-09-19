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
#include "framework/framework_manager.h"
#include "framework/timer_module.h"
#include "framework/log_util.h"

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

int main(int argc, char* argv[])
{
    std::string module_path;
    if (argc > 0)
    {
        const char* file = argv[0];
        const char* start = file;
        const char* file_name = file;

        while (file && *(file++) != '\0')
        {
            if ('/' == *file || '\\' == *file)
            {
                file_name = file + 1;
            }
        }
        module_path.assign( start, file_name );
    }

    framework::framework_manager::get_instance().run( nullptr );
    framework::framework_manager::get_instance().power_up();
    auto timer_module_ = std::dynamic_pointer_cast< framework::timer_module>(
        framework::framework_manager::get_instance().get_module_manager().get_module(
            framework::timer_module::s_timer_module_name ) );

    uint32_t timer1 = timer_module_->register_once_timer( std::bind( &once_timer, std::placeholders::_1, std::placeholders::_2 ),
        std::chrono::seconds( 2 ), "once_timer");

    uint32_t timer2 = timer_module_->register_timer( std::bind( &timer_handler_for_sometimes, std::placeholders::_1, std::placeholders::_2 ),
        std::chrono::seconds( 1 ), "10_times_timer", 10 );

    uint32_t timer3 = timer_module_->register_timer( std::bind( &usually_timer, std::placeholders::_1, std::placeholders::_2 ),
        std::chrono::seconds( 1 ), "usually_timer", 0 );

    std::future<void> fucture_ = promis_.get_future();
    fucture_.wait();

    std::cout << "Test done!\n";
}

