#include "cgguf.h"
#include "cutils.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    uint8_t  magic[4]; // `GGUF` encoded as 0x46554747
    uint32_t version; // should be at 3
    uint64_t tensor_count;
    uint64_t value_count;
} header_s;

typedef struct cgguf {
    const header_s * hdr;
    size_t size;
} cgguf_s;

cgguf_s * cgguf_open(const char *fname) {
    cgguf_s * ctx = 0;
    int fd = -1;
    header_s * hdr = MAP_FAILED;

    ctx = malloc(sizeof *ctx);
    if (ERR_ON(!ctx, "malloc"))
        goto fail;

    fd = open(fname, O_RDONLY);
    if (ERR_ON(fd < 0, "open: %s", ERRSTR))
        goto fail;

    struct stat st;
    int ret = fstat(fd, &st);
    if (ERR_ON(ret, "fstat: %s", ERRSTR))
        goto fail;
    size_t size = st.st_size;

    hdr = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ERR_ON(hdr == MAP_FAILED, "mmap: %s", ERRSTR))
        goto fail;

    // basic consitency checks for GGUF
    if (ERR_ON(size <= sizeof *hdr, "File is too small"))
        goto fail;
    if (ERR_ON(memcmp(hdr, "GGUF", 4) != 0, "invalid magic"))
        goto fail;

    *ctx = (cgguf_s) { .hdr = hdr, .size = size };

    // file descriptor is no longer needed
    close(fd);

    return ctx;

fail:
    if (hdr != MAP_FAILED) munmap(hdr, size);
    if (fd >= 0) close(fd);
    free(ctx);
    return 0;
}

void cgguf_drop(cgguf_s * ctx) {
    munmap((void*)ctx->hdr, ctx->size);
    free(ctx);
}
