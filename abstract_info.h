#pragma once
#include "lendable_element.h"

#include <string>

namespace framework
{

class abstract_information : public base_element
{

public:

    abstract_information(){}

    void set_name( std::string a_name )
    {
        m_name = a_name;
    }

    std::string const& get_name()const
    {
        return m_name;
    }

private:

    std::string m_name;
};

}

