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

private:

    void execute( std::shared_ptr<abstract_task> a_task );

    std::string m_task_schedule_helper_module_name;
};

}
