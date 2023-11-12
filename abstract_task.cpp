#include "abstract_task.h"
#include "log_util.h"

namespace framework
{

std::string source_position::to_string()const
{
    std::string str;
    str.push_back( '[' );
    if( m_file )
    {
        str.append( get_name_from_path( m_file ) );
        str.push_back( ':' );
        str.append( std::to_string( m_line ) );
        str.push_back( ']' );
    }
    else
    {
        str.push_back( ']' );
    }
    return str;
}

std::shared_ptr<abstract_task> abstract_task::clone()const
{
    std::shared_ptr<abstract_task> ret = std::make_shared<abstract_task>();
    copy_to( ret );
    return ret;
}

void abstract_task::copy_to( std::shared_ptr<abstract_task> a_tsk )const
{
    a_tsk->m_debug_info = m_debug_info;
    a_tsk->m_position = m_position;
    a_tsk->m_source_name = m_source_name;
    a_tsk->m_target_name = m_target_name;
    a_tsk->m_task_type = m_task_type;
}

}

