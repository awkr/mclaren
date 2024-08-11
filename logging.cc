#include "logging.h"
#include <cstdarg>
#include <ctime>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define PURPLE "\033[35m"
#define WHITE "\033[37m"
#define NORMAL "\033[0m"

static void log_print(const char *level, const char *format, va_list args) {
#if defined(PLATFORM_ANDROID)
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    struct tm *now = localtime(&tv.tv_sec);

    int32_t pid = getpid(); // process id

    uint64_t thread_id;
    pthread_threadid_np(nullptr, &thread_id);

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%d/%02d/%02d %02d:%02d:%02d.%03d %d %llu %s > %s\n",
             now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec,
             tv.tv_usec / 1000, pid, thread_id, level, format);
    vprintf(buffer, args);
#endif
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(WHITE "debug" NORMAL, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(GREEN "info" NORMAL, format, args);
    va_end(args);
}

void log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(PURPLE "warning" NORMAL, format, args);
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_print(RED "error" NORMAL, format, args);
    va_end(args);
}

void log_assert_failed(const char *func, const char *file, int line, const char *format, ...) {
#if defined(PLATFORM_ANDROID)
#else
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("assert failed: %s\n  at %s:%d:%s\n", buffer, file, line, func);
#endif
}
