#pragma once
#include "abstract_module.h"

namespace framework
{

class general_seq_task_runner_module : public abstract_module
{

public:

    general_seq_task_runner_module( std::string a_module_name = std::string() );

    void initialize()override;

    void deinitialize()override;

    void handle_task( std::shared_ptr<abstract_task> a_task )override;

    void handle_event( std::shared_ptr<framework_event> a_event )override;

};

}

