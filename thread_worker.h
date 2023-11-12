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

private:

    void run_impl( std::shared_ptr<abstract_worker> a_current );

    /**
     * Handle one task.
     * Return true then exit current thread
     */
    bool handle_task( std::shared_ptr<abstract_task> const& a_task );

    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    std::vector<std::shared_ptr<abstract_task>> m_tasks;
    std::chrono::steady_clock::time_point m_last_executing_time;

    std::thread m_thread;
    std::string m_thread_name;
    std::atomic_bool m_is_running;
};

}

