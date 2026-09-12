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
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "framework_export.h"

namespace framework
{

class FRAMEWORK_EXPORT thread_pool_watchdog
{
public:
    /**
     * A snapshot of a task currently monitored by the watchdog.
     */
    struct monitored_task
    {
        /** The identifier of the worker thread executing the task. */
        uint64_t thread_id = 0;

        /** The monotonic-clock time point at which task execution started. */
        int64_t started_at = 0;
    };

    /** The current operating state of the watchdog. */
    enum class state
    {
        /** No task is running and the watchdog is waiting indefinitely. */
        sleeping,

        /**
         * No task is running, but the configured idle delay has not elapsed.
         * The watchdog uses a timed condition-variable wait in this state and
         * does not continuously consume CPU time.
         */
        idle_observation,

        /** At least one worker thread is executing a task. */
        monitoring
    };

    /**
     * Called when a task exceeds the configured execution timeout.
     *
     * The callback runs on the watchdog thread and outside the watchdog lock.
     * It should return promptly so that other worker threads remain monitored.
     */
    using timeout_callback = std::function<void( monitored_task const& )>;

    thread_pool_watchdog();
    ~thread_pool_watchdog();

    thread_pool_watchdog( thread_pool_watchdog const& ) = delete;
    thread_pool_watchdog& operator=( thread_pool_watchdog const& ) = delete;

    /**
     * Reports that a worker thread is about to execute a task.
     *
     * This function starts the watchdog thread lazily, records the current
     * monotonic time, and changes the watchdog state to monitoring.
     *
     * @param a_thread_id Identifier of the worker thread executing the task.
     */
    void task_started( uint64_t a_thread_id );

    /**
     * Reports that a worker thread has finished executing its current task.
     *
     * When the last monitored task finishes, the watchdog enters the idle
     * observation state. Calling this function with an unmonitored thread ID
     * has no effect on the monitored-task collection.
     *
     * @param a_thread_id Identifier previously passed to task_started().
     */
    void task_finished( uint64_t a_thread_id );

    /**
     * Sets how long the watchdog observes an idle thread pool before sleeping.
     *
     * A new task arriving during this delay immediately returns the watchdog
     * to the monitoring state. A negative duration is treated as zero. The
     * observation uses a condition-variable wait and does not poll.
     *
     * @param a_delay Idle observation duration.
     */
    void set_idle_delay( std::chrono::milliseconds a_delay );

    /**
     * Sets the maximum execution time allowed for one task.
     *
     * A task still running after this duration is logged and passed to the
     * active timeout callback once. The default callback terminates the process
     * by calling std::abort(); a user-provided callback replaces that behavior.
     * Durations below one millisecond are clamped to one millisecond.
     *
     * @param a_timeout Maximum task execution duration.
     */
    void set_task_timeout( std::chrono::milliseconds a_timeout );

    /**
     * Sets the callback invoked when a monitored task times out.
     *
     * The callback replaces the default handler, which terminates the process
     * by calling std::abort(). Passing an empty callback restores the default
     * handler.
     *
     * @param a_callback Callback executed on the watchdog thread.
     */
    void set_timeout_callback( timeout_callback a_callback );

    /**
     * Returns a thread-safe snapshot of the current watchdog state.
     */
    state get_state()const;

    /**
     * Returns thread-safe snapshots of all tasks currently being monitored.
     */
    std::vector<monitored_task> monitored_tasks()const;

private:
    struct task_record
    {
        int64_t started_at;
        bool timeout_reported = false;
    };

    void default_timeout( monitored_task const& );
    void ensure_thread_started();
    void run();

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::unordered_map<uint64_t, task_record> m_tasks;
    std::chrono::milliseconds m_idle_delay{ 3000 };
    std::chrono::milliseconds m_task_timeout{ 10000 };
    std::chrono::steady_clock::time_point m_idle_since;
    timeout_callback m_timeout_callback;
    std::thread m_thread;
    state m_state = state::sleeping;
    bool m_stop = false;
};

}
