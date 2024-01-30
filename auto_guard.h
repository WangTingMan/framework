#pragma once
#include <functional>

namespace framework
{

class auto_guard
{

public:

    auto_guard
        (
        std::function<void()> a_entry_fun,
        std::function<void()> a_exit_fun
        )
        : m_exit_fun( a_exit_fun )
    {
        a_entry_fun();
    }

    auto_guard( std::function<void()> a_exit_fun )
        : m_exit_fun( a_exit_fun )
    {

    }

    ~auto_guard()
    {
        if( m_exit_fun )
        {
            m_exit_fun();
        }
    }

    auto_guard( const auto_guard& ) = delete;
    auto_guard& operator=( const auto_guard& ) = delete;

private:

    std::function<void()> m_exit_fun;
};

}

