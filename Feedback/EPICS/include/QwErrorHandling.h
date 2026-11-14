#pragma once
#include <iostream>
#include <string>
#include <stdexcept>

#if __cplusplus >= 202002L
// ==========================================
// C++20 and Later Implementation
// ==========================================
#include <source_location>

[[noreturn]] inline void throw_with_location(
    const std::string& message,
    const std::source_location location = std::source_location::current())
{   
    std::string full_msg = std::string(location.file_name()) + ":"
                         + std::to_string(location.line()) + " in "
                         + location.function_name() + " -> " + message;      
    throw std::runtime_error(full_msg);
}                                                                                   

#define THROW_ERROR(msg) throw_with_location(msg)

#else
// ==========================================
// Pre-C++20 Fallback Implementation
// ==========================================
[[noreturn]] inline void throw_with_location_fallback(
    const std::string& message,
    const char* file,
    int line,
    const char* func)
{   
    std::string full_msg = std::string(file) + ":"
                         + std::to_string(line) + " in "
                         + func + " -> " + message;      
    throw std::runtime_error(full_msg);
}                                                                                   

#define THROW_ERROR(msg) throw_with_location_fallback(msg, __FILE__, __LINE__, __func__)

#endif
