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
#include <memory>
#include <string>

#include "framework_export.h"

namespace framework
{

class abstract_task;
class abstract_module;

/**
 * User can inherit this class to specific how to schedule the module's task.
 * If just create this module_task_handler's instance, then all moudles which used
 * this instance will execute all the task in same worker( thread ) sequentially.
 */
class FRAMEWORK_EXPORT module_task_handler
{

public:

    module_task_handler( std::string a_task_handler_name = std::string() );

    virtual ~module_task_handler();

    virtual void handle( std::shared_ptr<abstract_task> a_task );

    virtual std::optional<int> get_current_executing_thread_id()const;

private:

    void execute( std::shared_ptr<abstract_task> a_task );

    std::string m_task_schedule_helper_module_name;
};

}
