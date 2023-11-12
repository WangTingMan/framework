#include "information_manager.h"

namespace framework
{

information_manager::information_manager()
{

}

bool information_manager::register_information
    (
    std::shared_ptr<abstract_information> a_information
    )
{
    bool ret = false;
    std::lock_guard<std::mutex> locker( m_mutex );
    if( !m_informations.contains( a_information->get_name() ) )
    {
        m_informations[a_information->get_name()] = a_information;
        ret = true;
    }
    return ret;
}

std::shared_ptr<abstract_information> information_manager::get_informaion
    (
    std::string const& a_name
    )
{
    std::lock_guard<std::mutex> locker( m_mutex );
    auto it = m_informations.find( a_name );
    if( it != m_informations.end() )
    {
        return it->second;
    }
    return nullptr;
}

}

