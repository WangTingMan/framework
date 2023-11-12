#pragma once
#include "abstract_module.h"

namespace framework
{

class task_runner_module : public abstract_module
{

public:

    task_runner_module();

    void initialize()override;

    void deinitialize()override;

    void handle_task( std::shared_ptr<abstract_task> a_task )override;

    void handle_event( std::shared_ptr<framework_event> a_event )override;

};

}

