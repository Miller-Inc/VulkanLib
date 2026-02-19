//
// Created by James Miller on 2/5/2026.
//

#pragma once
#include <string>

inline std::string operator/(const std::string& lhs, const std::string& rhs)
{
    if (lhs.empty())
        return rhs;
    if (rhs.empty())
        return lhs;

    std::string result = lhs;
    if (result.back() != '/' && result.back() != '\\')
        result += '/';
    result += rhs;
    return result;
}

inline std::string operator/(const char* lhs, const std::string& rhs)
{
    return std::string(lhs) / rhs;
}

inline std::string operator/(const std::string& lhs, const char* rhs)
{
    return lhs / std::string(rhs);
}