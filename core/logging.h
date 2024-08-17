#pragma once

#include <cstdio>

#define STR(x) #x

#define ASSERT(cond) \
    if (!(cond)) { \
        log_assert_failed(__FUNCTION__, __FILE__, __LINE__, STR(cond)); \
        fflush(stdout); \
        __builtin_trap(); \
    }

#define ASSERT_MESSAGE(cond, fmt, ...) \
    if (!(cond)) { \
        log_assert_failed(__FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        fflush(stdout); \
        __builtin_trap(); \
    }

void log_debug(const char *format, ...);

void log_info(const char *format, ...);

void log_warning(const char *format, ...);

void log_error(const char *format, ...);

void log_assert_failed(const char *func, const char *file, int line, const char *format, ...);
