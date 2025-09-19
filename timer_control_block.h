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
#include <chrono>
#include <memory>
#include <string>

#include <functional>

#include "framework_export.h"

//#define DEBUG_TIMER_MODULE

namespace framework
{

class FRAMEWORK_EXPORT timer_control_block
{

public:

    /**
     * The first parameter is the timer id, the second parameter is the timer's name
     */
    using timeout_callback = std::function<void( uint32_t, std::string )>;

    timer_control_block();

    void set_timeout_callback( timeout_callback a_expire_callback )
    {
        m_callback = a_expire_callback;
    }

    timeout_callback const& get_timeout_callback()const
    {
        return m_callback;
    }

    void set_interval( uint32_t a_interval );

    int get_interval()
    {
        return m_milliseconds;
    }

    void set_trigger_times( uint32_t a_trigger_time )
    {
        m_trigger_times = a_trigger_time;
    }

    /**
     * Get how many times the timer will be triggered.
     * Return 0xFFFF means the timer do not have the limit, that is the timer will
     * be trigger for infinite times.
     */
    uint32_t get_remain_trigger_timers();

    int64_t const& get_time_to_execute()const
    {
        return m_timeToExecute;
    }

    void set_timer_id( uint32_t a_id )
    {
        m_timer_id = a_id;
    }

    uint32_t const& get_timer_id()const
    {
        return m_timer_id;
    }

    void set_timer_name( std::string a_name )
    {
        m_name = std::move( a_name );
    }

    std::string const& get_timer_name()const
    {
        return m_name;
    }

    void timer_triggered();

    /**
     * The first time to schedule this timer should invoke this
     */
    void start_schedule();

    void set_handle_module( std::string a_module_name )
    {
        m_handle_module = std::move( a_module_name );
    }

    std::string const& get_handle_module()const
    {
        return m_handle_module;
    }

private:

    uint32_t    m_timer_id = 0;               //!< Timer id
    int         m_milliseconds = 0;           //!< Timer duration
    uint32_t    m_trigger_times = 0;          //!< How many times will be triggered.
    uint32_t    m_triggered_times = 0;        //!< How many times this timer has been triggered.
    int64_t     m_timeToExecute = 0;          //!< Time to next fire the timer(s)
    bool        m_combine = false;            //!< Are we allowed to combine timers
    std::string m_name;                       //!< The timer entry name
    timeout_callback m_callback;         //!< When the timer expire then m_callback will be invoked.
                                         // return true if want to cancel this timer.
    int64_t     m_timer_start_time = 0;  //!< the time when this timer started.
    std::string m_handle_module;         //!< Which module to handle the callback. If empty then will directly call the callback
};

}

