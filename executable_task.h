#pragma once
#include "abstract_task.h"
#include "abstract_module.h"

namespace framework
{

class executable_task : public abstract_task
{

public:

    executable_task( std::function<bool()> a_fun )
    {
        set_task_type( task_type::executable_task );
        m_target_name = abstract_module::s_task_runner_module_name;
        m_task = a_fun;
    }

    executable_task()
    {
    }

    void set_fun
        (
        std::function<void()> a_fun,
        std::string a_target_module
        )
    {
        if( !m_task )
        {
            set_task_type( task_type::executable_task );
            m_target_name = a_target_module;
            if( a_target_module.empty() )
            {
                m_target_name = abstract_module::s_task_runner_module_name;
            }
            m_task_no_return = a_fun;
        }
    }

    /**
    * return true to exit current thread
    */
    bool run_task()
    {
        if( m_task )
        {
            return m_task();
        }

        if( m_task_no_return )
        {
            m_task_no_return();
            return false;
        }

        return false;
    }

    bool operator()()
    {
        return run_task();
    }

private:

    std::function<bool()> m_task;
    std::function<void()> m_task_no_return;

};

}

