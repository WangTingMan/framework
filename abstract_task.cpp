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

