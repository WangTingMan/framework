#pragma once
#include <string>
#include <functional>
#include <cstdint>

#include "framework_export.h"

namespace framework
{

struct source_position
{
    const char* m_file = nullptr;
    uint64_t m_line = 0;
    source_position
        (
        const char* a_file = nullptr,
        uint64_t a_line = 0
        )
        : m_file( a_file )
        , m_line( a_line )
    {

    }

    std::string to_string()const;

};

#define source_here source_position( __FILE__, __LINE__ )

enum class task_type : uint16_t
{
    normal_type = 0,
    executable_task = 1,
    framework_event = 2
};

class FRAMEWORK_EXPORT abstract_task
{

public:

    abstract_task(){}

    virtual ~abstract_task(){}

    std::string const& get_target_module()const
    {
        return m_target_name;
    }

    virtual void set_target_module( std::string a_module )
    {
        m_target_name = a_module;
    }

    std::string const& get_source_module()const
    {
        return m_source_name;
    }

    void set_source_module( std::string a_module )
    {
        m_source_name = a_module;
    }

    void set_task_type( task_type a_type )
    {
        m_task_type = a_type;
    }

    task_type const& get_task_type()const
    {
        return m_task_type;
    }

    void set_debug_info( std::string a_debug )
    {
        m_debug_info = a_debug;
    }

    std::string const& get_debug_info()const
    {
        return m_debug_info;
    }

    void set_position( source_position a_pos )
    {
        m_position = a_pos;
    }

    source_position const& get_position()const
    {
        return m_position;
    }

    virtual std::shared_ptr<abstract_task> clone()const;

    /**
     * copy all member value into a_tsk
     */
    void copy_to( std::shared_ptr<abstract_task> a_tsk )const;

protected:

    std::string m_target_name;
    std::string m_source_name;
    std::string m_debug_info;
    source_position m_position;
    task_type m_task_type = task_type::normal_type;
};

}

