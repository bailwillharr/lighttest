#pragma once

#include <format>
#include <iostream>

#include <iCUESDK/iCUESDK.h>

#define CHECKCORSAIR(fun) ::corsairCheckError(fun, #fun)

template <typename... Args>
[[noreturn]] inline void die(std::format_string<Args...> fmt, Args&&... args)
{
    std::cerr << "FATAL ERROR: " << std::format(fmt, std::forward<Args>(args)...) << "\n";
    std::abort();
}

const char* corsairErrToString(CorsairError err);

const char* corsairSessionStateToString(CorsairSessionState session_state);

void corsairCheckError(CorsairError err, const char* function_name);

// returns nullptr if keyboard not found
const CorsairDeviceId* findKeyboard();

template <>
struct std::formatter<CorsairError> {
    constexpr auto parse(std::format_parse_context& ctx) const { return ctx.begin(); }
    auto format(CorsairError err, std::format_context& ctx) const { return std::format_to(ctx.out(), "{}", corsairErrToString(err)); }
};

template <>
struct std::formatter<CorsairSessionState> {
    constexpr auto parse(std::format_parse_context& ctx) const { return ctx.begin(); }
    auto format(CorsairSessionState session_state, std::format_context& ctx) const { return std::format_to(ctx.out(), "{}", corsairSessionStateToString(session_state)); }
};

template <>
struct std::formatter<CorsairVersion> {
    constexpr auto parse(std::format_parse_context& ctx) const { return ctx.begin(); }
    auto format(CorsairVersion ver, std::format_context& ctx) const { return std::format_to(ctx.out(), "{}.{}.{}", ver.major, ver.minor, ver.patch); }
};