#pragma once

#include <stdlib.h>
#include <stddef.h>

typedef struct {
    void * data;
    size_t used;
    size_t left;
} ibuf_s;

#define IBUF_INIT (ibuf){0}

static inline void * ibuf_grow(ibuf_s * ib, size_t grow) {
    char * data = ib->data;
    if (ib->left < grow) {
        size_t size = ib->used + grow;
        size += size / 2;
        data = realloc(data, size);
        if (!data)
            return 0;
        ib->data = data;
        ib->left = new_size - ib->used;
    }
    void * ptr = data + ib->used;
    ib->used += grow;
    ib->left -= grow;
    return ptr;
}

static inline void * ibuf_drop(ibuf_s * ib) {
    void * data = ib->data;
    *ib = (ibuf_s){ 0 };
    return data;
}

