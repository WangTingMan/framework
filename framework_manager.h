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

private:

    void init( std::function< std::vector<std::shared_ptr<framework::abstract_module>>()> a_module_maker );

    module_manager m_module_manager;
    thread_manager m_thread_manager;
    information_manager m_info_manager;

    mutable std::shared_mutex m_mutex;
    bool m_is_running = false;
};

}

