#pragma once
#include "abstract_task.h"

namespace framework
{

enum class event_type : uint16_t
{
    invlaid_type = 0x00,
    power_on = 0x01,
    power_off = 0x02,
    power_status_changed = 0x03, // the parameter m_module_name indicated which module's power status changed.
    derived_type = 0x04, // the detail type is derived from framework.
};

class framework_event : public abstract_task
{

public:

    framework_event()
    {
        m_task_type = task_type::framework_event;
    }

    std::shared_ptr<abstract_task> clone()const override
    {
        std::shared_ptr<framework_event> task = std::make_shared<framework_event>();
        copy_to( task );
        return task;
    }

    void copy_to( std::shared_ptr<framework_event> a_tsk )const
    {
        a_tsk->m_event_type = m_event_type;
        a_tsk->m_module_name = m_module_name;
        abstract_task::copy_to( a_tsk );
    }

    event_type m_event_type = event_type::invlaid_type;
    std::string m_module_name; // See tye power_status_changed
};

}

