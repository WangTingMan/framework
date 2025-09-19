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
#include <condition_variable>
#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <vector>

#include "abstract_task.h"
#include "abstract_worker.h"

namespace framework
{

class thread_worker : public abstract_worker
{

public:

    thread_worker();

    ~thread_worker();

    void run
        (
        std::shared_ptr<abstract_worker> a_current,
        bool a_accupy_cureent_thread = false
        )override;

    void post_task( std::shared_ptr<abstract_task> a_task )override;

    void post_task( std::vector<std::shared_ptr<abstract_task>> a_tasks )override;

    bool is_idle_for_long_time()override;

    void exit_later()override;

    uint64_t work_thread_id()override;

private:

    void run_impl( std::shared_ptr<abstract_worker> a_current );

    /**
     * Handle one task.
     * Return true then exit current thread
     */
    bool handle_task( std::shared_ptr<abstract_task> const& a_task );

    uint64_t m_thread_id = 0;

    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    std::vector<std::shared_ptr<abstract_task>> m_tasks;
    std::chrono::steady_clock::time_point m_last_executing_time;

    std::thread m_thread;
    std::string m_thread_name;
    std::atomic_bool m_is_running;
};

}

