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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <string>

#include "abstract_module.h"
#include "framework_export.h"
#include "timer_control_block.h"

namespace framework
{

class FRAMEWORK_EXPORT timer_module : public abstract_module
{

public:

    timer_module();

    static int64_t get_system_booting_time();

    static std::string to_booting_time_stamp( int64_t a_booting_time );

    void initialize()override;

    void deinitialize()override;

    void handle_task( std::shared_ptr<abstract_task> a_task )override;

    void handle_event( std::shared_ptr<framework_event> a_event )override;

    /**
     * Register a periodic timer. After the timer expired, then a_expire_callback
     * will be invoked. If a_interval equals zero, then a_expire_callback will be
     * an asynchronous task and this timer cannot be canceled and should ignore the
     * timer id.
     * a_expire_callback: the timeour callback. If the return value of this callback
     * is true, then the timer will be canceled, otherwise this callback will be
     * invoked at next a_interval time.
     * a_interval: the interval time for this timer.
     * a_trigger_times: how mant times this timer will be triggered. If a_trigger_times
     * equals zero, then the timer will be keep running until canceled.
     * return: the timer id for the timer created.
     */
    uint32_t register_timer
        (
        timer_control_block::timeout_callback a_expire_callback,
        std::chrono::milliseconds a_interval,
        uint32_t a_trigger_times = 0,
        std::string a_handle_module = ""
        );

    /**
     * Overwrite version. This version adds a parameter named a_timer_name.
     */
    uint32_t register_timer
        (
        timer_control_block::timeout_callback a_expire_callback,
        std::chrono::milliseconds a_interval,
        std::string a_timer_name,
        uint32_t a_trigger_times = 0,
        std::string a_handle_module = ""
        );

    uint32_t register_once_timer
        (
        timer_control_block::timeout_callback a_expire_callback,
        std::chrono::milliseconds a_interval,
        std::string a_timer_name = "",
        std::string a_handle_module = ""
        )
    {
        return register_timer( a_expire_callback, a_interval, a_timer_name, 1, a_handle_module );
    }

    /**
     * Modify the timer's interval
     */
    void reset_timer
        (
        uint32_t a_id,
        std::chrono::milliseconds a_interval
        );

    /**
     * Cancel the timer identified by a_timer_id. If the timer callback has been
     * scheduled, then the timer callback cannot cancelled.
     */
    void undregister_timer( uint32_t a_timer_id );

private:

    void handle_timer_expired();

    void make_schedule_task_if_need( int64_t a_front_time_to_execute );

    std::recursive_mutex m_mutex;   // Protect m_timers
    std::list<std::shared_ptr<timer_control_block>> m_timers;

    std::atomic_uint32_t m_timer_count = 1;

    std::mutex m_condition_mutex;
    std::condition_variable m_condition;
    std::atomic_bool m_condition_waiting = false;
    uint64_t m_weak_up_time = 0xFFFFFFFF; // The time from system up time to wake up
};

}

