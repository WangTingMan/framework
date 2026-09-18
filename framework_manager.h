/*
  Copyright (c) 2009-2026 WangTingMan <75142601@qq.com>

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
#include "module_manager.h"
#include "thread_manager.h"
#include "information_manager.h"

#include <shared_mutex>

#include "framework_export.h"

namespace framework
{

class FRAMEWORK_EXPORT framework_manager
{

public:

    static framework_manager& get_instance();

    framework_manager();

    thread_manager& get_thread_manager()
    {
        return m_thread_manager;
    }

    module_manager& get_module_manager()
    {
        return m_module_manager;
    }

    information_manager& get_info_manager()
    {
        return m_info_manager;
    }

    void run
        (
        std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker,
        bool a_occupy_current_thread = false
        );

    void power_up();

    bool is_running()const;

    /**
     * @brief Registers a batch of task types and allocates a contiguous range of task‑type IDs.
     *
     * Allocates a contiguous block of task‑type IDs according to the requested count.
     * Returns the starting ID of the allocated range. The internal offset @c m_next_task_type
     * is advanced automatically. This method is thread‑safe with exclusive locking.
     * The valid allocated ID range is [return_value, return_value + a_count - 1].
     *
     * @param a_count Number of consecutive task‑type IDs to allocate. Must be greater than zero.
     * @return uint16_t Starting ID of the allocated task‑type range.
     *
     * @note Advances @c m_next_task_type for subsequent allocations.
     * @warning Be aware of uint16_t overflow risk. If @c m_next_task_type + a_count exceeds
     *          @c UINT16_MAX, ID values will wrap around and cause incorrect ID assignments.
     * @threadsafe Thread‑safe; acquires exclusive ownership of @c m_mutex.
     */
    uint16_t register_task_type( uint16_t a_count = 1 );

private:

    void init( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker );

    module_manager m_module_manager;
    thread_manager m_thread_manager;
    information_manager m_info_manager;

    mutable std::shared_mutex m_mutex;
    bool m_is_running = false;
    uint16_t m_next_task_type = 0;
};

}

