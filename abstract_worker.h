#pragma once
#include <memory>

#include "abstract_task.h"

namespace framework
{

class abstract_worker : std::enable_shared_from_this<abstract_worker>
{

public:

    virtual ~abstract_worker(){}

    virtual void run
        (
        std::shared_ptr<abstract_worker> a_current,
        bool a_accupy_cureent_thread = false
        ) = 0;

    virtual void post_task( std::shared_ptr<abstract_task> a_task ) = 0;

    virtual void post_task( std::vector<std::shared_ptr<abstract_task>> a_tasks ) = 0;

    virtual bool is_idle_for_long_time() = 0;

    virtual void exit_later() = 0;

private:

};

}

