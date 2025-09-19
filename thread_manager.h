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
#include "abstract_worker.h"
#include "abstract_module.h"
#include <vector>
#include <mutex>
#include <unordered_map>

namespace framework
{

class FRAMEWORK_EXPORT thread_manager
{

public:

    /**
     * Module task schedule control block
     */
    struct module_task_cb
    {
        std::string module_name;
        abstract_module::module_type module_type_value = abstract_module::module_type::sequence_executing;
        std::list<std::shared_ptr<abstract_task>> pending_tasks;
        std::shared_ptr<abstract_worker> m_executing_worker;
    };

    /**
     * Max thread number. That is thread manager can manage how many threads.
     */
    constexpr static uint8_t s_max_worker_num = 6;

    /**
     * A task can wait for executing time is s_max_task_time_out ms.
     */
    constexpr static uint8_t s_max_task_time_out = 20;

    /**
     * Run thread pool
     */
    void run( bool a_occupy_current_thread = false );

    void post_task( std::function<void()> a_tsk );

    void post_delay_task
        (
        std::chrono::milliseconds a_delay_time,
        std::function<void()> a_tsk
        );

    /**
     * Post one task into thread pool
     */
    void post_task( std::shared_ptr<abstract_task> a_task );

    /**
     * post some tasks into thread pool
     */
    void post_task( std::vector<std::shared_ptr<abstract_task>> a_tasks );

    /**
     * Internal use. push a idle thread into thread poll which is waiting for
     * task to do.
     */
    void push_idle_worker( std::shared_ptr<abstract_worker> a_worker );

    /**
     * Register module types. Thread manager will schedule these tasks depend on module type.
     */
    void register_module_type
        (
        abstract_module::module_type a_type,
        std::string a_module_name
        );

    uint64_t get_scheduled_thread_id( std::string const& a_moudle_name )const;

    /**
     * Remove a worker from list. That is, that work is about to quit.
     */
    void remove_worker( std::shared_ptr<abstract_worker> a_worker );

    static std::string const& get_current_thread_module_owner();

    static void set_current_thread_module_owner( std::string a_module_name );

private:

    /**
     * Schedule threads.
     * 1. Determine if new thread need added or not.
     *   a. We need add new thread if there are many works need to do
     *   b. We need add new thread if the first of task m_work_need_assign
     *      have waiting for s_max_task_time_out ms.
     * 2. Determine if a thread need remove or not.
     * 3. Other things to do.
     */
    void schedule_workers();

    /**
     * Find a idle worker or allocate a new worker
     */
    std::shared_ptr<abstract_worker> find_idle_worker();

    /**
     * assign a_task to a_worker
     */
    void assign_work
        (
        std::shared_ptr<abstract_worker>& a_worker,
        std::shared_ptr<abstract_task>& a_task
        );

    /**
     * assign a_task to a_worker
     */
    void assign_work
        (
        std::shared_ptr<abstract_worker>& a_worker,
        std::list<std::shared_ptr<abstract_task>>& a_task
        );

    /**
     * If there is a long idle worker, we need dismiss it and release some system resource
     */
    void dismiss_long_idle_worker();

    void schedule_sequence_task
        (
        module_task_cb& a_task_cb,
        std::shared_ptr<abstract_task> a_task
        );

    void schedule_immediately_task
        (
        std::shared_ptr<abstract_task> a_task,
        std::string const& a_module
        );

    void schedule_concurrently_task
        (
        std::shared_ptr<abstract_task> a_task
        );

    void schedule_handler_task
        (
        std::shared_ptr<abstract_task> a_task
        );

    mutable std::recursive_mutex m_mutex;
    std::unordered_map<std::string, module_task_cb> m_modules_shcedule;

    uint32_t m_schedule_timer_id = 0;
    std::vector<std::shared_ptr<abstract_worker>> m_idle_worker; // The workers have no work to do
    std::vector<std::shared_ptr<abstract_worker>> m_working_worker; // The workers are working
    std::vector<std::shared_ptr<abstract_task>>   m_work_need_assign;
};

}

