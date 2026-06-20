#pragma once

//-----------------------------------------------------------------------------
// Logger.h
// Lightweight debug logging.
// In release builds (NDEBUG defined) all calls compile to nothing.
// In debug builds output goes to Citra's console (printf → svcOutputDebugString).
//-----------------------------------------------------------------------------

#include <cstdio>
#include <cstdarg>

class Logger {
public:
    // Call once at startup.
    static void init();

    // Print a formatted debug message. Compiled away in release.
    static void log(const char* fmt, ...);

    // Print a formatted warning. Always compiled in.
    static void warn(const char* fmt, ...);

    // Print a formatted error. Always compiled in.
    static void error(const char* fmt, ...);

private:
    static bool s_initialized;
};

//-----------------------------------------------------------------------------
// Convenience macros — use these in source files
//-----------------------------------------------------------------------------
#ifndef NDEBUG
#define LOG(fmt, ...)   Logger::log(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)   ((void)0)
#endif

#define WARN(fmt, ...)  Logger::warn(fmt, ##__VA_ARGS__)
#define ERR(fmt, ...)   Logger::error(fmt, ##__VA_ARGS__)
