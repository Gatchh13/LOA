//-----------------------------------------------------------------------------
// Logger.cpp
//-----------------------------------------------------------------------------

#include "Logger.h"
#include <3ds.h>
#include <cstring>

bool Logger::s_initialized = false;

void Logger::init() {
    s_initialized = true;
    // In Citra, printf output appears in the terminal that launched the emulator.
    // On hardware, output is visible via GDB stub or simply dropped.
    LOG("Logger initialized");
}

void Logger::log(const char* fmt, ...) {
#ifndef NDEBUG
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("[LOG] %s\n", buf);
#else
    (void)fmt;
#endif
}

void Logger::warn(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("[WARN] %s\n", buf);
}

void Logger::error(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("[ERR] %s\n", buf);
}

