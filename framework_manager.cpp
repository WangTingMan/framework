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

#include "framework_manager.h"
#include "framework_event.h"
#include "timer_module.h"
#include "log_util.h"

#include <mutex>

namespace framework
{

framework_manager& framework_manager::get_instance()
{
    static framework_manager instance;
    return instance;
}

bool  framework_manager::is_running()const
{
    std::shared_lock<std::shared_mutex> locker( m_mutex );
    return m_is_running;
}

void framework_manager::run
    (
    std::function<std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker,
    bool a_occupy_current_thread
    )
{
    std::lock_guard<std::shared_mutex> locker( m_mutex );
    if( m_is_running )
    {
        return;
    }
    init( std::move( a_module_maker ) );
    m_is_running = true;
    m_thread_manager.run( a_occupy_current_thread );
}

void framework_manager::power_up()
{
    std::shared_ptr<framework_event> event_ = std::make_shared<framework_event>();
    event_->m_event_type = event_type::power_on;
    m_thread_manager.post_task( event_ );
}

void framework_manager::init( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker )
{
    m_module_manager.load_modules( std::move( a_module_maker ) );
    m_module_manager.initialize();
}

}

