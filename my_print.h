#pragma once

#include <iostream>
#include <format>

template <typename... Args>
inline void myPrint(std::format_string<Args...> fmt, Args&&... args)
{
    std::cerr << std::format(fmt, std::forward<Args>(args)...) << "\n";
}
