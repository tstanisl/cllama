#include "cgguf.h"
#include "cutils.h"

#include <assert.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    u64 left;
    const char * data;
} stream_s;

static u64 type_size(cgguf_type_e type) {
    switch (type) {
    case CGGUF_TYPE_UINT8:   return 1;
    case CGGUF_TYPE_INT8:    return 1;
    case CGGUF_TYPE_UINT16:  return 2;
    case CGGUF_TYPE_INT16:   return 2;
    case CGGUF_TYPE_UINT32:  return 4;
    case CGGUF_TYPE_INT32:   return 4;
    case CGGUF_TYPE_FLOAT32: return 4;
    case CGGUF_TYPE_BOOL:    return 1;
    case CGGUF_TYPE_STRING:  return -1;
    case CGGUF_TYPE_ARRAY:   return -1;
    case CGGUF_TYPE_UINT64:  return 8;
    case CGGUF_TYPE_INT64:   return 8;
    case CGGUF_TYPE_FLOAT64: return 8;
    default: return 0;
    };
}

static bool scan_(stream_s * s, void * ptr, u64 size) {
    if (ERR_ON(s->left < size, "file too short"))
        return 0;
    if (ptr) memcpy(ptr, s->data, size);
    s->data = (char*)s->data + size;
    s->left -= size;
    return 1;
}

#define scan(s,p) scan_((s), (p), sizeof *(p))
#define skip(s,n) scan_((s), 0, (n))

static bool scan_type(stream_s * s, cgguf_type_e * type) {
    u32 u;
    if (!scan(s, &u))
        return 0;
    if (ERR_ON(u >= 13, "invalid type: %" PRIu32, u))
        return 0;
    *type = u;
    return 1;
}

static bool scan_str(stream_s * s, cgguf_str_s * p) {
    if (ERR_ON(!scan(s, &p->size), "scan"))
        return 0;
    p->data = s->data;
    return skip(s, p->size);
}

static bool scan_arr(stream_s * s, cgguf_arr_s * a) {
    if (!scan_type(s, &a->type))
        return 0;
    if (!scan(s, &a->left))
        return 0;
    a->data = s->data;
    return 1;
}

static bool skip_arr(stream_s * s, cgguf_arr_s * a) {
    //printf("%s %d %d\n", __func__, (int)a->left, (int)a->type);
    u64 elem_size = type_size(a->type);
    assert(elem_size > 0);
    // fixed element types
    if (elem_size != (u64)-1)
        return skip(s, elem_size * a->left); // safe?
    if (a->type == CGGUF_TYPE_STRING) {
        for (u64 i = 0; i < a->left; ++i) {
            u64 slen;
            if (!scan(s, &slen))
                return 0;
            if (!skip(s, slen))
                return 0;
        }
        return 1;
    } else {
        assert(a->type == CGGUF_TYPE_ARRAY);
        for (u64 i = 0, cnt = a->left; i < cnt; ++i) {
            printf("i=%d\n", (int)i);
            if (ERR_ON(scan_arr(s, a), "scan_arr"))
                return 0;
            if (ERR_ON(skip_arr(s, a), "skip_arr"))
                return 0;
        }
        return 1;
    }
}

static bool scan_val(stream_s * s,
    cgguf_type_e type, cgguf_val_u * v
) {
    u64 size = type_size(type);
    assert(size > 0);
    if (size != (u64)-1)
        return scan_(s, v, size);
    if (type == CGGUF_TYPE_STRING)
        return scan_str(s, &v->str);
    assert(type == CGGUF_TYPE_ARRAY);
    return scan_arr(s, &v->arr);
}

static bool scan_keyval(stream_s * s, cgguf_keyval_s * kv) {
    if (!scan_str(s, &kv->key))
        return 0;
    if (!scan_type(s, &kv->type))
        return 0;
    return scan_val(s, kv->type, &kv->val);
}

static bool scan_tensor(stream_s * s, cgguf_tensor_s * t) {
    if (!scan_str(s, &t->name))
        return 0;

    u32 ndims;
    if (!scan(s, &ndims))
        return 0;
    if (ERR_ON(ndims > CGGUF_MAX_DIMS, "too many dims"))
        return 0;
    for (u32 i = 0; i < CGGUF_MAX_DIMS; ++i)
        if (i < ndims) {
            if (!scan(s, &t->dims[i]))
                return 0;
        } else {
            t->dims[i] = 1;
        }

    u32 dfmt;
    if (!scan(s, &dfmt))
        return 0;

    if (ERR_ON(dfmt >= CGGUF_DFMT_COUNT, "invalid dfmt %" PRIu32, dfmt))
        return 0;
    t->dfmt = dfmt;

    if (!scan(s, &t->_off))
        return 0;

    return 1;
}

typedef struct {
    cgguf_s base;
    char * data;
    size_t size;
} ctx_s;

const cgguf_s * cgguf_open(const char *fname) {
    int fd = -1;
    char * data = MAP_FAILED;
    cgguf_keyval_s * keyvals = 0;
    cgguf_tensor_s * tensors = 0;
    ctx_s * ctx = 0;

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

    stream_s s = { .left = size, .data = data };

    struct {
        u8  magic[4]; // `GGUF` encoded as 0x46554747
        u32 version; // should be at 3
        u64 n_tensors;
        u64 n_keyvals;
    } hdr;

    if (!scan(&s, &hdr))
        goto fail;

    if (ERR_ON(memcmp(hdr.magic, "GGUF", 4) != 0, "not gguf"))
        goto fail;
    if (ERR_ON(hdr.version != 3, "bad version"))
        goto fail;

    keyvals = calloc(hdr.n_keyvals, sizeof *keyvals);
    if (ERR_ON(!keyvals, "malloc"))
        goto fail;

    tensors = calloc(hdr.n_tensors, sizeof *tensors);
    if (ERR_ON(!tensors, "malloc"))
        goto fail;
    
    ctx = malloc(sizeof *ctx);
    if (ERR_ON(!ctx, "malloc"))
        goto fail;

    u32 alignment = 32;
    // scan key-value pairs
    for (u64 i = 0; i < hdr.n_keyvals; ++i) {
        cgguf_keyval_s * kv = &keyvals[i];
        if (ERR_ON(!scan_keyval(&s, kv), "scan_keyval %" SCNu64, i))
            goto fail;
        if (kv->type == CGGUF_TYPE_ARRAY)
            if (ERR_ON(!skip_arr(&s, &kv->val.arr), "skip_arr"))
                goto fail;
        if (keyvals[i].type == CGGUF_TYPE_UINT32 &&
            cgguf_strequal(kv->key, "general.alignment")
        ) alignment = kv->val.u32;
    }

    if (ERR_ON(alignment == 0 || alignment % 8 != 0,
               "invalid alignment %" PRIu32, alignment))
        goto fail;

    // scan tensors
    for (u64 i = 0; i < hdr.n_tensors; ++i)
        if (ERR_ON(!scan_tensor(&s, &tensors[i]),
                   "scan_tensor %" SCNu64, i))
            goto fail;

    // compute tensor_offset
    u64 tensor_offset = (size - s.left + alignment - 1) / alignment * alignment;
    for (u64 i = 0; i < hdr.n_tensors; ++i) {
        u64 off = tensors[i]._off;
        if (ERR_ON(off % alignment != 0, "invalid offset"))
            goto fail;
        off += tensor_offset;
        if (ERR_ON(off > size, "offset overflows file"))
            goto fail;
        tensors[i].data = data + off;
    }

    // file descriptor is no longer needed
    close(fd);

    // update tensors

    *ctx = (ctx_s) {
        .base = {
            .alignment = alignment,
            .n_keyvals = hdr.n_keyvals,
            .keyvals = keyvals,
            .n_tensors = hdr.n_tensors,
            .tensors = tensors,
        },
        .data = data,
        .size = size,
    };

    return &ctx->base;

fail:
    free(keyvals);
    free(tensors);
    free(ctx);
    if (data != MAP_FAILED) munmap(data, size);
    if (fd >= 0) close(fd);
    return 0;
}

void cgguf_drop(const cgguf_s * base) {
    ctx_s * ctx = container_of(base, ctx_s, base);
    munmap((void*)ctx->data, ctx->size);
    free((void*)ctx->base.keyvals);
    free((void*)ctx->base.tensors);
    free(ctx);
}

bool cgguf_read_val(cgguf_arr_s* a, cgguf_val_u* v) {
    if (a->left) {
        stream_s s = { .data = a->data, .left = -1 };
        scan_val(&s, a->type, v);
        a->left--;
        a->data = s.data;
        return 1;
    }
    return 0;
}

void cgguf_skip_arr(cgguf_arr_s* a) {
    stream_s s = { .data = a->data, .left = -1 };
    skip_arr(&s, a);
    a->left = 0;
    a->data = s.data;
}

bool cgguf_strequal(cgguf_str_s a, const char * b) {
    size_t alen = a.size;
    size_t blen = strlen(b);
    return alen == blen && memcmp(a.data, b, alen) == 0;
}
