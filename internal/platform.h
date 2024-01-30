#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "framework/framework_export.h"

namespace framework
{

FRAMEWORK_EXPORT void set_thread_name( const std::string& a_name );

FRAMEWORK_EXPORT uint64_t get_time_stamp();

FRAMEWORK_EXPORT uint64_t get_current_thread_id();

FRAMEWORK_EXPORT std::u8string convert( std::string const& a_source );

FRAMEWORK_EXPORT std::string convert( std::u8string const& a_source );

FRAMEWORK_EXPORT std::wstring convert_to_wstring( std::string a_str );

FRAMEWORK_EXPORT std::string convert_to_string( std::wstring a_str );

FRAMEWORK_EXPORT void base64_encode( char const* a_buffer, uint16_t a_size, std::string* output );

FRAMEWORK_EXPORT bool base64_decode( const std::string& input, std::vector<char>& output );

FRAMEWORK_EXPORT std::vector<std::string> split_string( std::string const& a_source, char a_split );

FRAMEWORK_EXPORT std::string trim_string( std::string const& a_source );

FRAMEWORK_EXPORT std::string current_call_stack();

}

