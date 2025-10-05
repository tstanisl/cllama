#include "cgguf.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

typedef struct {
    uint32_t magic; // `GGUF` encoded as 0x46554747
    uint32_t version; // should be at 3
    uint64_t tensor_count;
    uint64_t value_count;
} header_s;

typedef struct cgguf {
    const header_s * hdr;
    int fd;
} cgguf_s;

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

cgguf_s * cgguf_open(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (ERR_ON(fd < 0, "Could not open '%s': %s", fname, ERRSTR))
        return 0;

    return 0;
}
