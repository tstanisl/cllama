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

#define ERR(...) msg_(__FILE__, __LINE__, "error", __VA_ARGS__)
#define ERR_ON(cnd, ...) (cnd ? ERR(__VA_ARGS__), 1 : 0)
#define ERRSTR strerror(errno)

#define ASSERT(...) \
    if (__VA_ARGS__); \
    else (msg_(__FILE__, __LINE__, "assert", "%s", # __VA_ARGS__), abort())

typedef uint64_t u64;
typedef  int64_t i64;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint16_t u16;
typedef  int16_t i16;
typedef  uint8_t u8;
typedef   int8_t i8;
typedef    float f32;
typedef   double f64;
