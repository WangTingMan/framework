#pragma once
#include "abstract_info.h"
#include "framework_export.h"

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <string>

namespace framework
{

class base_element;

class FRAMEWORK_EXPORT information_manager
{

public:

    information_manager();

    bool register_information
        (
        std::shared_ptr<abstract_information> a_information
        );

    std::shared_ptr<abstract_information> get_informaion( std::string const& a_name );

    template<typename T>
    std::shared_ptr<T> get_detail_information( std::string const& a_name )
    {
        auto info = get_informaion( a_name );
        return std::dynamic_pointer_cast< T >( info );
    }

private:

    std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<abstract_information>> m_informations;
};

}

