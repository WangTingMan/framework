#pragma once
#include <memory>
#include <mutex>

namespace framework
{

class base_element
{

public:

    virtual ~base_element(){}
};

template<typename T>
class lendable_element
{

public:

    lendable_element( T* a_element )
    {
        set( a_element );
    }

    template<typename... Types>
    lendable_element( Types... a_args)
    {
        m_element = std::make_unique<T>(a_args...);
        m_initialized = true;
    }

    lendable_element(){}

    void set( T* a_element )
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        if( m_initialized )
        {
            throw std::logic_error( "already initialized!" );
        }
        else
        {
            m_initialized = true;
            m_element.reset( a_element );
        }
    }

    std::unique_ptr<T> lend_out()
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        if( m_initialized )
        {
            return std::move( m_element );
        }
        else
        {
            throw std::logic_error( "not initialized!" );
        }
    }

    void return_back( std::unique_ptr<T> a_element )
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        if( m_initialized )
        {
            m_element = std::move( a_element );
        }
        else
        {
            throw std::logic_error( "not initialized!" );
        }
    }

    bool lendable()const
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        return lendable_internal();
    }

    bool operator()()const
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        return lendable_internal();
    }

    operator bool()const
    {
        std::lock_guard<std::mutex> locker( m_mutex );
        return lendable_internal();
    }

private:

    bool lendable_internal()const
    {
        return m_initialized && static_cast< bool >( m_element );
    }

    mutable std::mutex m_mutex;
    bool m_initialized = false;
    std::unique_ptr<T> m_element;
};

template<typename T>
class auto_return_back
{

public:

    auto_return_back( lendable_element<T>& a_lendable, std::unique_ptr<T>& a_ptr )
        : m_lendable( a_lendable )
        , m_ptr( a_ptr )
    {

    }

    ~auto_return_back()
    {
        m_lendable.return_back( std::move( m_ptr ) );
    }

private:

    lendable_element<T>& m_lendable;
    std::unique_ptr<T>& m_ptr;
};

template<typename T>
class unique_return_back
{

public:

    unique_return_back() {}

    explicit unique_return_back( lendable_element<T>& a_lendable, std::unique_ptr<T>& a_ptr )
        : m_lendable( std::addressof( a_lendable ) )
        , m_owns( true )
        , m_hold_ptr( std::addressof( a_ptr ) )
    {
    }

    ~unique_return_back() noexcept
    {
        return_back();
    }

    explicit operator bool() const noexcept
    {
        return m_owns;
    }

    void return_back()
    {
        if( m_owns )
        {
            m_lendable->return_back( std::move( *m_hold_ptr ) );
            m_owns = false;
        }
    }

    unique_return_back( const unique_return_back& ) = delete;
    unique_return_back& operator=( const unique_return_back& ) = delete;

private:

    lendable_element<T>* m_lendable = nullptr;
    std::unique_ptr<T>* m_hold_ptr = nullptr;
    bool m_owns = false;
};

using base_auto_return_back = auto_return_back<base_element>;
using base_unique_return_back = unique_return_back<base_element>;

}

