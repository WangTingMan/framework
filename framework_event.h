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

