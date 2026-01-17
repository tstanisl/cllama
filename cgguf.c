#include "cgguf.h"
#include "cutils.h"

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

cgguf_s * cgguf_open(const char *fname) {
    int fd = open(fname, O_RDONLY);
    if (ERR_ON(fd < 0, "Could not open '%s': %s", fname, ERRSTR))
        return 0;


    return 0;
}
