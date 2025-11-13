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

#pragma once

#include <functional>
#include <string>
#include <memory>
#include <sstream>
#include <cstdint>

#include "framework_export.h"

namespace framework
{

enum class log_level : uint8_t
    {
    verbose,
    info,
    debug,
    warning,
    error,
    fatal
    };

using log_callback =
    std::function<bool
        (
        const char* /* the source file name*/,
        int /* source line*/,
        log_level,
        std::string const&
        )>;

void FRAMEWORK_EXPORT set_log_callback( log_callback a_callBack );

void FRAMEWORK_EXPORT rotate_log( std::string a_log_dir, std::string a_log_file_name, uint32_t a_max_file_num );

void FRAMEWORK_EXPORT set_default_log_location( std::string a_log_dir, std::string a_log_file );

const char* get_name_from_path( const char* a_path );

FRAMEWORK_EXPORT std::ostream& operator<<( std::ostream& a_os, log_level a_level );

template<size_t N>
constexpr size_t count_format_specs( const char( &a_fmt_string )[N] )
{
    size_t count = 0;
    char current;
    char next;
    for (int i = 0; i < N - 1; ++i)
    {
        current = a_fmt_string[i];
        next = a_fmt_string[i+1];
        if (current == '%')
        {
            if (next == '\0')
            {
                break;
            }

            if (next != '%')
            {
                ++count;
            }
        }
    }
    return count;
}

class FRAMEWORK_EXPORT log_impl
{

    friend class util_logger;

public:

    struct log_content
    {
        const char* m_file_name = nullptr;
        bool m_is_log_eater = false;
        int m_line_number = 0;
        log_level m_level = log_level::info;
    };

    log_impl() {}

    template <typename Types>
    log_impl& operator<<( Types&& args )
    {
        if( m_logger_content && !m_logger_content->m_is_log_eater )
        {
            m_string_stream << args;
        }
        return *this;
    }

    log_impl( log_impl&& a_right ) noexcept
    {
        m_logger_content = std::move( a_right.m_logger_content );
        m_string_stream.swap( a_right.m_string_stream );
    }

    log_impl& operator=( log_impl&& a_right ) noexcept
    {
        m_logger_content = std::move( a_right.m_logger_content );
        m_string_stream.swap( a_right.m_string_stream );
        return *this;
    }

    ~log_impl();

private:

    void set_log_contnet( std::unique_ptr<log_content>&& a_content )
    {
        m_logger_content = std::move( a_content );
    }

    std::stringstream m_string_stream;
    std::unique_ptr<log_content> m_logger_content;
};

class FRAMEWORK_EXPORT util_logger
{
public:

    util_logger( const char* a_file_name, int a_line_number, bool a_is_log_eater,
        log_level a_level )
        : m_file_name( a_file_name ),
        m_is_log_eater( a_is_log_eater ),
        m_line_number( a_line_number ),
        m_level( a_level )
    {
    }

    util_logger( const char* a_file_name, int a_line_number, log_level a_level )
        : m_file_name( a_file_name ),
        m_is_log_eater( is_ignore_level( a_level ) ),
        m_line_number( a_line_number ),
        m_level( a_level )
    {
    }

    log_impl logging()
    {
        log_impl log_;
        if( !m_is_log_eater )
        {
            std::unique_ptr<log_impl::log_content> content;
            content = std::make_unique<log_impl::log_content>();
            content->m_file_name = m_file_name;
            content->m_is_log_eater = m_is_log_eater;
            content->m_line_number = m_line_number;
            content->m_level = m_level;
            log_.set_log_contnet( std::move( content ) );
        }
        return log_;
    }

    template<size_t N, typename ...Types>
    void logging( const char (&a_format_str)[N], Types... a_args)
    {
        constexpr size_t actual_count = sizeof...(a_args);
        size_t expected_count = count_format_specs( a_format_str );

        if (actual_count >= expected_count)
        {
            logging_with_format( actual_count, a_format_str, a_args... );
        }
        else
        {
            framework::util_logger( m_file_name, m_line_number, framework::log_level::error )
                .logging() << "the a_format_str expected " << expected_count << " parameters. but "
                << actual_count << " parameters!";
            logging_with_format( actual_count, "%s", a_format_str );
        }
    }

    static void set_log_level( log_level level = log_level::verbose );

    static inline bool is_ignore_level( log_level level ) { return level < s_logLevel; }

    static log_level get_log_level();

private:
    void logging_with_format( int a_parameter_count, const char* msg, ... ) const;
    const char* m_file_name = nullptr;
    bool m_is_log_eater = false;
    int m_line_number = 0;
    log_level m_level = log_level::fatal;
    static log_level s_logLevel;
};

}  // namespace framework

#define LogUtilVerbose                                               \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::verbose \
                   ).logging
#define LogUtilInfo                                               \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::info \
                   ).logging
#define LogUtilDebug                                              \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::debug\
                   ).logging
#define LogUtilWarning                                              \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::warning\
                   ).logging
#define LogUtilError                                               \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::error \
                   ).logging
#define LogUtilFatal                                               \
  framework::util_logger(__FILE__, __LINE__, framework::log_level::fatal \
                   ).logging

#define LogUtilIgnore while (false)                                  \
  framework::util_logger(__FILE__, __LINE__, true, framework::log_level::info  \
                   ).logging

