#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void msg_(const char * fname, int line, const char * level, const char * fmt, ...) {
    fprintf(stderr, "%s:%d: %s: ", fname, line, level);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fputc('\n', stderr);
}

#define ERR_ON(cnd, ...) \
    (cnd ? msg_(__FILE__, __LINE__, "error", __VA_ARGS__), 1 : 0)
#define ERRSTR strerror(errno)

#define ASSERT(...) \
    if (__VA_ARGS__); \
    else (msg_(__FILE__, __LINE__, "assert", "%s", # __VA_ARGS__), abort())


