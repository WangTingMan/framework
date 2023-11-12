#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace framework
{

void set_thread_name( const std::string& a_name );

uint64_t get_time_stamp();

uint64_t get_current_thread_id();

std::u8string convert( std::string const& a_source );

std::string convert( std::u8string const& a_source );

void base64_encode( char const* a_buffer, uint16_t a_size, std::string* output );

bool base64_decode( const std::string& input, std::vector<char>& output );

std::vector<std::string> split_string( std::string const& a_source, char a_split );

std::string trim_string( std::string const& a_source );

std::string current_call_stack();

}

