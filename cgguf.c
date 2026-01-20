#include "cgguf.h"
#include "cutils.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

_Static_assert(sizeof(cgguf_value_type_e) == sizeof(u32), "invalid size");

typedef struct cgguf {
    char * data;
    size_t size;
    const void * tensors;
    u64 alignment;
    u64 nkeyvals;
    u64 ntensors;
    const cgguf_str_s * keys[];
} cgguf_s;

typedef struct {
    u64 size;
    u64 left;
    void * data;
} stream_s;

static bool fetch_(stream_s * s, void * b, u64 size) {
    if (s->left < size) {
        ERR("failed to fetch %zu byte at offset %zu", size, s->size - s->left);
        return 0;
    }
    if (b) memcpy(b, s->data, size);
    s->data = (char*)s->data + size;
    s->left -= size;
    return 1;
}

#define fetch(s,b) fetch_((s), (b), sizeof *(b))

static int fixed_vtype_size(cgguf_value_type_e type) {
    switch (type) {
    case CGGUF_VALUE_TYPE_BOOL:    return 1;
    case CGGUF_VALUE_TYPE_UINT8:   return 1;
    case CGGUF_VALUE_TYPE_INT8:    return 1;
    case CGGUF_VALUE_TYPE_UINT16:  return 2;
    case CGGUF_VALUE_TYPE_INT16:   return 2;
    case CGGUF_VALUE_TYPE_UINT32:  return 4;
    case CGGUF_VALUE_TYPE_INT32:   return 4;
    case CGGUF_VALUE_TYPE_FLOAT32: return 4;
    case CGGUF_VALUE_TYPE_UINT64:  return 8;
    case CGGUF_VALUE_TYPE_INT64:   return 8;
    case CGGUF_VALUE_TYPE_FLOAT64: return 8;
    default: return 0;
    };
}

static bool skip_str(stream_s * s) {
    u64 len;
    return fetch(s, &len) && fetch_(s, 0, len);
}

static bool skip_arr(stream_s * s) {
    cgguf_value_type_e vtype;
    u64 len;
    if (!fetch(s, &vtype) || !fetch(s, &len))
        return 0;
    if (vtype == CGGUF_VALUE_TYPE_STRING) {
        for (u64 i = 0; i < len; ++i)
            if (!skip_str(s))
                return 0;
        return 1;
    }
    if (vtype == CGGUF_VALUE_TYPE_ARRAY) {
        for (u64 i = 0; i < len; ++i)
            if (!skip_arr(s))
                return 0;
        return 1;
    }
    int vtype_size = fixed_vtype_size(vtype);
    if (ERR_ON(vtype_size <= 0, "unknown vtype"))
        return 0;
    return fetch_(s, 0, len * vtype_size);
}

static bool skip_val(stream_s * s, cgguf_value_type_e vtype) {
    if (vtype == CGGUF_VALUE_TYPE_STRING)
        return skip_str(s);
    if (vtype == CGGUF_VALUE_TYPE_ARRAY)
        return skip_arr(s);
    int vtype_size = fixed_vtype_size(vtype);
    if (ERR_ON(vtype_size <= 0, "unknown vtype"))
        return 0;
    return fetch_(s, 0, vtype_size);
}

static bool scan(cgguf_s * g, stream_s * s) {
    size_t kv_count = g->nkeyvals;
    cgguf_value_type_e vtype;
    for (size_t i = 0; i < kv_count; ++i) {
        g->keys[i] = s->data;
        if (ERR_ON(!skip_str(s) ||
                   !fetch(s, &vtype) ||
                   !skip_val(s, vtype),
                   "when parsing keyval[%zu]", i))
            return 0;
        // TODO: general.alignment
        printf("kv[%zu]=%*.s\n", i, (int)g->keys[i]->len, g->keys[i]->str);
    }
    return 1;
}

cgguf_s * cgguf_open(const char *fname) {
    cgguf_s * ctx = 0;
    int fd = -1;
    void * data = MAP_FAILED;

    fd = open(fname, O_RDONLY);
    if (ERR_ON(fd < 0, "open: %s", ERRSTR))
        goto fail;

    struct stat st;
    int ret = fstat(fd, &st);
    if (ERR_ON(ret, "fstat: %s", ERRSTR))
        goto fail;
    size_t size = st.st_size;

    data = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ERR_ON(data == MAP_FAILED, "mmap: %s", ERRSTR))
        goto fail;

    struct {
        u8  magic[4]; // `GGUF` encoded as 0x46554747
        u32 version; // should be at 3
        u64 ntensors;
        u64 nkeyvals;
    } hdr;

    stream_s strm = { .data = data, .left = size, .size = size };
    if (!fetch(&strm, &hdr))
        goto fail;
    // basic consitency checks for GGUF
    if (ERR_ON(memcmp(hdr.magic, "GGUF", 4) != 0, "invalid magic"))
        goto fail;
    if (ERR_ON(hdr.version != 3, "invalid version"))
        goto fail;

    ctx = malloc(sizeof *ctx + hdr.nkeyvals * sizeof *ctx->keys);
    if (ERR_ON(!ctx, "malloc"))
        goto fail;

    *ctx = (cgguf_s) {
        .data = data,
        .size = size,
        .alignment = 32,
        .nkeyvals = hdr.nkeyvals,
        .ntensors = hdr.ntensors,
    };

    if (!scan(ctx, &strm))
        goto fail;

    // file descriptor is no longer needed
    close(fd);

    return ctx;

fail:
    if (data != MAP_FAILED) munmap(data, size);
    if (fd >= 0) close(fd);
    free(ctx);
    return 0;
}

void cgguf_drop(cgguf_s * ctx) {
    munmap((void*)ctx->data, ctx->size);
    free(ctx);
}

int cgguf_strequal(const cgguf_str_s * a, const char * b) {
    size_t alen = a->len;
    size_t blen = strlen(b);
    return alen == blen && memcmp(a->str, b, alen) == 0;
}
#if 0
cgguf_keyval_s cgguf_keyval_start(cgguf_s * c) {
    r
    return (unpack_keyval(c, c->hdr + 1);
}
#endif
