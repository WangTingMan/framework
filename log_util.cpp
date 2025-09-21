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

#include "log_util.h"
#include "internal/platform.h"
#include "timer_module.h"
#include "thread_manager.h"
#include "utils.h"

#if __has_include( <base/time/time.h> )
#ifndef USE_CHROME_BASE_LIBRARY
#define USE_CHROME_BASE_LIBRARY
#endif
#endif

#ifdef USE_CHROME_BASE_LIBRARY
#include <base/time/time.h>
#include <base/threading/thread.h>
#include <base/threading/platform_thread.h>
#include <base/strings/string_util.h>
#include <base/files/file_util.h>
#include <base/debug/stack_trace.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <sstream>
#include <string>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <unordered_map>

#define LOGLINE_SIZE 1024

#ifndef LOG_FOLDER
#define LOG_FOLDER "E:/VCLAB/BluetoothStack/x64/Debug/"
#endif

#ifndef LOG_FILE_NAME
#define LOG_FILE_NAME "frame_work"
#endif

#ifndef MAX_LINE_IN_SINGLE_FILE
#define MAX_LINE_IN_SINGLE_FILE 200000
#endif

#ifndef MAX_LOG_FILE
#define MAX_LOG_FILE 5
#endif

#ifdef USE_CHROME_BASE_LIBRARY
bool libchrome_log_callback( int level, const char* file, int line,
    size_t message_start, const std::string& str );
#endif

struct log_control_block
{

public:

    log_control_block()
    {
        m_log_line_callback = std::bind( &log_control_block::record_log,
            this, std::placeholders::_1 );
        m_log_dir = LOG_FOLDER;
        m_log_file_name = LOG_FILE_NAME;
#ifdef USE_CHROME_BASE_LIBRARY
        logging::SetLogMessageHandler( libchrome_log_callback );
#endif
    }

    framework::log_callback get_log_callback()const
    {
        std::shared_lock<std::shared_mutex> locker( m_mutex );
        return m_callBack;
    }

    std::function<void( std::string )> get_log_line_callback()const
    {
        std::shared_lock<std::shared_mutex> locker( m_mutex );
        return m_log_line_callback;
    }

    const char* get_name_from_full_path( const char* a_file ) const
    {
        std::shared_lock<std::shared_mutex> locker( m_mutex );
        auto it = m_file_names.find( a_file );
        if( it != m_file_names.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    void record_log( std::string log )
    {
        std::unique_lock<std::shared_mutex> locker( m_mutex );
        if( !m_log_file.is_open() )
        {
            framework::rotate_log( m_log_dir, m_log_file_name, MAX_LOG_FILE );
            //open a new log file.
            std::string fileName;
            fileName.append( m_log_dir ).append( m_log_file_name ).append( ".log" );
            m_log_file.open( fileName, std::ios::out | std::ios::trunc );
            if( !m_log_file.is_open() )
            {
                return;
            }
        }

        m_log_file << log << std::flush;
        if( ++m_current_file_lines > MAX_LINE_IN_SINGLE_FILE )
        {
            m_log_file.close();
            m_current_file_lines = 0;
        }
        locker.unlock();
    }

    void record_log_cache( std::string log )
    {
        bool is_flush = false;
        std::unique_lock<std::shared_mutex> locker( m_mutex );
        m_cached_logs.push_back( std::move( log ) );
        if( m_cached_logs.size() >= s_max_cached_line )
        {
            is_flush = true;
        }
        else
        {

        }

        locker.unlock();
        if( is_flush )
        {
            recode_log_cache_impl();
        }
    }

    void recode_log_cache_impl()
    {
        std::unique_lock<std::shared_mutex> locker( m_mutex );
        if( !m_log_file.is_open() )
        {
            framework::rotate_log( m_log_dir, m_log_file_name, MAX_LOG_FILE );
            //open a new log file.
            std::string fileName;
            fileName.append( m_log_dir ).append( m_log_file_name ).append( ".log" );
            m_log_file.open( fileName, std::ios::out | std::ios::trunc );
            if( !m_log_file.is_open() )
            {
                return;
            }
        }

        std::string log_lines;
        for( auto& ele : m_cached_logs )
        {
            log_lines.append( std::move( ele ) );
        }
        m_log_file << log_lines << std::flush;
        m_current_file_lines += m_cached_logs.size();
        m_cached_logs.clear();

        if( m_current_file_lines > MAX_LINE_IN_SINGLE_FILE )
        {
            m_log_file.close();
            m_current_file_lines = 0;
        }
        locker.unlock();
    }

    mutable std::shared_mutex m_mutex;
    framework::log_callback m_callBack;
    std::unordered_map<const char*, const char*> m_file_names;
    std::ofstream m_log_file;
    int m_current_file_lines = 0;
    std::string m_log_dir;
    std::string m_log_file_name;
    std::function<void( std::string )> m_log_line_callback;

    std::vector<std::string> m_cached_logs;
    static constexpr int s_max_cached_line = 30;
};

static log_control_block s_log_cb;

framework::log_level framework::util_logger::s_logLevel = framework::log_level::verbose;

const char* framework::get_name_from_path( const char* a_path )
{
    const char* file = a_path;
    const char* file_name = s_log_cb.get_name_from_full_path( a_path );
    if( file_name )
    {
        return file_name;
    }

    while( file && *( file++ ) != '\0' )
    {
        if( '/' == *file || '\\' == *file )
        {
            file_name = file + 1;
        }
    }

    if( !file_name )
    {
        file_name = "Unkown";
    }

    std::lock_guard<std::shared_mutex> locker( s_log_cb.m_mutex );
    s_log_cb.m_file_names[a_path] = file_name;

    return file_name;
}

static std::string get_timestamp_string()
{
    std::string stamp_str = framework::timer_module::to_booting_time_stamp(
        framework::timer_module::get_system_booting_time() );

    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::time_point_cast<std::chrono::seconds>( now );
    auto us = std::chrono::duration_cast<std::chrono::microseconds>( now - now_sec ).count();

    std::time_t now_time_t = std::chrono::system_clock::to_time_t( now_sec );
    std::tm local_tm;
#if defined(_MSC_VER) || defined(_WIN32)
    localtime_s( &local_tm, &now_time_t );  // Windows
#else
    localtime_r( &now_time_t, &local_tm );  // Linux/Unix
#endif

    std::array<char, 50> buffer;
    char* ptr = buffer.data();
    char* buffer_end = buffer.data() + buffer.size();
    *ptr = '[';
    ++ptr;

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_year + 1900, 4 ) )
    {
        return "format error";
    }
    *ptr++ = '-';

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_mon + 1, 2 ) )
    {
        return "format error";
    }
    *ptr++ = '-';

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_mday, 2 ) )
    {
        return "format error";
    }
    *ptr++ = ' ';

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_hour, 2 ) )
    {
        return "format error";
    }
    *ptr++ = ':';

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_min, 2 ) )
    {
        return "format error";
    }
    *ptr++ = ':';

    if( !format_number_with_padding( ptr, buffer_end, local_tm.tm_sec, 2 ) )
    {
        return "format error";
    }
    *ptr++ = '.';
    if( !format_number_with_padding( ptr, buffer_end, us, 6 ) )
    {
        return "format error";
    }
    *ptr++ = ']';

    stamp_str.append( buffer.data(), ptr );

    return stamp_str;
}

static std::string& get_current_thread_id_string()
{
    static thread_local std::string id_str = std::format( "{:0>5d}", framework::get_current_thread_id() );
    return id_str;
}

framework::log_impl::~log_impl()
{
    bool ret = true;
    std::string _log_line;
    if( m_logger_content )
    {
        auto log_callback = s_log_cb.get_log_callback();
        if( log_callback )
        {
            ret = log_callback( m_logger_content->m_file_name, m_logger_content->m_line_number,
                    m_logger_content->m_level, m_string_stream.str() );
        }

        if( ret )
        {
            const char* file = get_name_from_path( m_logger_content->m_file_name );

            std::stringstream ss;
            ss << get_timestamp_string() <<
                "[" << m_logger_content->m_level << "]["
               << get_current_thread_id_string() << "] [" << file
               << ":" << m_logger_content->m_line_number
               << "] ";
            if( !thread_manager::get_current_thread_module_owner().empty() )
            {
                ss << "[" << thread_manager::get_current_thread_module_owner()
                   << "] ";
            }
            ss << m_string_stream.str();

            _log_line = std::move( ss ).str();
            if( '\n' != _log_line.back() )
            {
                _log_line.push_back( '\n' );
            }

            bool isFatal = log_level::fatal == m_logger_content->m_level;
            if( isFatal )
            {
                // Include a stack trace on a fatal, unless a debugger is attached.
                _log_line.append( current_call_stack() );
            }

            s_log_cb.get_log_line_callback()( std::move( _log_line ) );

            if( isFatal )
            {
                // Crash the process to generate a dump.
                std::abort();
            }
        }
        else
        {
            if( log_level::fatal == m_logger_content->m_level )
            {
                std::abort();
            }
        }

    }
}

std::ostream& framework::operator<< ( std::ostream& os,
    framework::log_level level )
{
    switch( level )
    {
    case framework::log_level::verbose:
        os << "V";
        break;
    case framework::log_level::info:
        os << "I";
        break;
    case framework::log_level::debug:
        os << "D";
        break;
    case framework::log_level::warning:
        os << "W";
        break;
    case framework::log_level::error:
        os << "E";
        break;
    case framework::log_level::fatal:
        os << "F";
        break;
    default:
        break;
    }
    return os;
}

void framework::set_log_callback( log_callback a_callBack )
{
    std::lock_guard<std::shared_mutex> locker( s_log_cb.m_mutex );
    s_log_cb.m_callBack = a_callBack;
}

void framework::set_default_log_location( std::string a_log_dir, std::string a_log_file )
{
    std::lock_guard<std::shared_mutex> locker( s_log_cb.m_mutex );
    s_log_cb.m_log_dir = std::move( a_log_dir );
    s_log_cb.m_log_file_name = std::move( a_log_file );
}

static inline std::string PringFormatLog( const char* a_format, va_list ap )
{
    if( !a_format || !*a_format )
    {
        return "";
    }

    char pBuffer[LOGLINE_SIZE];
    ( void )vsnprintf( pBuffer, LOGLINE_SIZE - 1, a_format, ap );
    return std::string( pBuffer );
}

void framework::util_logger::logging( const char* msg, ... ) const
{
    if( m_is_log_eater )
    {
        return;
    }

    std::string logStr;
    va_list ap;
    va_start( ap, msg );
    logStr = PringFormatLog( msg, ap );
    va_end( ap );

    log_impl logging;
    std::unique_ptr<log_impl::log_content> content;
    content = std::make_unique<log_impl::log_content>();
    content->m_file_name = m_file_name;
    content->m_is_log_eater = m_is_log_eater;
    content->m_line_number = m_line_number;
    content->m_level = m_level;
    logging.set_log_contnet( std::move( content ) );
    logging << logStr;
}

void framework::util_logger::set_log_level( framework::log_level level )
{
    s_logLevel = level;
}

framework::log_level framework::util_logger::get_log_level() { return s_logLevel; }

void framework::rotate_log( std::string a_logDir, std::string a_logFileName, uint32_t a_maxFileNum )
{
#ifdef USE_CHROME_BASE_LIBRARY
    const std::string& constLogDir = a_logDir;
    const std::string& constLogFileName = a_logFileName;

    std::string fileNameTemplate;
    fileNameTemplate.append( constLogDir )
        .append( constLogFileName )
        .append( "$1." )
        .append( "log" );
    base::StringPiece convertStr( fileNameTemplate );
    uint32_t fileCout = 0;

    if( a_maxFileNum < 1u )
    {
        a_maxFileNum = 1u;
    }

    for( fileCout = 0; fileCout < a_maxFileNum; ++fileCout )
    {
        std::string file;
        if( fileCout > 0 )
        {
            std::vector<std::string> substs;
            substs.emplace_back( std::to_string( fileCout ) );
            file = base::ReplaceStringPlaceholders( convertStr, substs, nullptr );
        }
        else
        {
            file.clear();
            file.append( constLogDir ).append( constLogFileName ).append( ".log" );
        }

        base::FilePath path( file );
        if( base::PathExists( path ) )
        {
            continue;
        }
        else
        {
            break;
        }
    }

    base::FilePath folder( constLogDir );
    while( fileCout > 0 )
    {
        std::vector<std::string> substs;
        substs.emplace_back( std::to_string( fileCout ) );
        std::string newName =
            base::ReplaceStringPlaceholders( convertStr, substs, nullptr );
        std::string originName;
        if( fileCout > 1 )
        {
            substs.clear();
            substs.emplace_back( std::to_string( fileCout - 1 ) );
            originName = base::ReplaceStringPlaceholders( convertStr, substs, nullptr );
        }
        else
        {
            originName.clear();
            originName.append( constLogDir ).append( constLogFileName ).append( ".log" );
        }

        base::FilePath newFilePath( newName );
        if( base::PathExists( newFilePath ) )
        {
            base::File::Info fileInfo;
            base::GetFileInfo( newFilePath, &fileInfo );
            if( !fileInfo.is_directory )
            {
                base::DeleteFile( newFilePath, false );
            }
        }
        base::File::Error error;
        base::ReplaceFile( base::FilePath( originName ), newFilePath, &error );
        --fileCout;
    }
#endif
}

#ifdef USE_CHROME_BASE_LIBRARY
bool libchrome_log_callback( int a_level, const char* a_file, int a_line,
    size_t a_message_start, const std::string& a_str )
{
    framework::log_level level = framework::log_level::verbose;
    switch( a_level )
    {
#ifdef BASE_LOG_DEBUG_LEVEL_DEFINED
    case logging::LOG_DEBUG:
        level = framework::log_level::Debug;
        break;
#endif
    case logging::LOG_VERBOSE:
        level = framework::log_level::verbose;
        break;
    case logging::LOG_INFO:
        level = framework::log_level::info;
        break;
    case logging::LOG_WARNING:
        level = framework::log_level::warning;
        break;
    case logging::LOG_ERROR:
        level = framework::log_level::error;
        break;
    case logging::LOG_FATAL:
        level = framework::log_level::fatal;
        break;
    case logging::LOG_NUM_SEVERITIES:
        /* Current not sure the usage of LOG_NUM_SEVERITIES, so just handle it as
         * verbose level.
         */
        level = framework::log_level::verbose;
        break;
    default:
        break;
    }
    std::string logStr;
    if( a_str.size() > a_message_start )
    {
        logStr = a_str.substr( a_message_start );
    }

    framework::util_logger( a_file, a_line, level ).logging() << logStr;

    return true;
}

#endif
