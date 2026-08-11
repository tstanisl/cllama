#include "ctokenizer.h"

#include "cutils.h"
#include "hmap.h"

typedef struct {
    u32 a, b, t;
} merge_s;

typedef struct ctokenizer {
    u32 n_tokens;
    char    * data;
    u32     * start;
    merge_s * merge;
} ctokenizer_s;

typedef struct {
    void * data;
    size_t used;
    size_t left;
} ibuf_s;

#define IBUF_INIT (ibuf){0}

static void * ibuf_grow(ibuf_s * ib, size_t grow) {
    char * data = ib->data;
    if (ib->left < grow) {
        size_t size = ib->used + grow;
        size += size / 2;
        data = realloc(data, size);
        if (ERR_ON(!data, "realloc"))
            return 0;
        ib->data = data;
        ib->left = new_size - ib->used;
    }
    void * ptr = data + ib->used;
    ib->used += grow;
    ib->left -= grow;
    return ptr;
}

static void * ibuf_drop(ibuf_s * ib) {
    void * data = ib->data;
    *ib = (ibuf_s){ 0 };
    return data;
}

static u64 strhash(u32 len, const char str[len]) {
    // FNV hash
    u64 hash = 0xcbf29ce484222325;
    for (u32 i = 0; i < len; ++i) {
        hash *= 0x00000100000001b3;
        hash ^= str[i];
    }
    return hash;
}

static uint32_t hash_token(uint32_t idx, const void* t_) {
    ctokenizer_s * t = t_;
    u32 * s = t->start;
    return strhash(s[idx + 1] - s[idx], t->data + s[idx]);
}

static u32 hash2x32(u32 a, u32 b) {
    u32 h = (a * 0x9e3779b9) ^ b; // combine
    h ^= h >> 16;                 // avalanche (Murmur3 fmix32)
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

ctokenizer_h ctokenizer_init(
    ctokenizer_type_e type,
    int n_tokens,
    ctokenizer_entry_s get(void*, int), void * priv
) {
    size_t size = 0;
    for (int i = 0; i < n_tokens; ++i)
        size += sizeof(token_s) + get(priv, i).size;

    ctokenizer_s t = {
        .data  = malloc(size),
        .start = calloc(n_tokens + 1, sizeof *t.start),
    };
    ctokenizer_s * res = malloc(sizeof *res);
    hmap_h hm = 0;
    ibuf_s ib = IBUF_INIT;

    if (ERR_ON(!t.data || !t.start || !res, "malloc"))
        goto fail;

    char * head = t->data;
    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = get(priv, i);
        // todo: handle byte-tokens
        memcpy(head, e.data, e.size);
        t.start[i] = head - t.data;
        head += e.size;
    }
    start[n_tokens] = head; // sentinel

    hm = hmap_init(n_tokens, hash_token, &t);
    if (ERR_ON(!hm, "hmap_init"))
        goto fail;

    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = get(priv, i);
        for (u32 p = 1; p + 1 < e.size; ++p) {
            u32 a = hmap_search(&hmap, strhash(p, e.data), tcmp, &hm);
            if (a == HMAP_NONE) continue;
            u32 b = hmap_search(&hmap, strhash(e.size - p, e.data + p), tcmp, &hm);
            if (b == HMAP_NONE) continue;
            merge_s * m = ibuf_grow(&ib, sizeof *m);
            if (ERR_ON(!m, "ibuf_grow"))
                goto fail;
            *m = (merge_s) { a, b, i };
        }
    }

    // mapping from string to token is no longer needed
    hmap_drop(hm);
    hm = 0;




    data = calloc(1, size);
    if (ERR_ON(!data, "malloc"))
        goto fail;

    int ret = hmap_init(&hmap, n_tokens);
    if (ERR_ON(ret, "hmap_init"))
        goto fail;

    char * head = data;
    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = get(priv, i);
        token_s * t = (void*) head;
        t->size = e.size;
        memcpy(t->data, e.data, e.size);
        hmap_insert(&hm, strhash(e.size, e.data), ~(head - data));
        head += e.size;
    }

    // create merges
    
    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = get(priv, i);
        for (u32 p = 1; p + 1 < e.size; ++p) {
            u32 a = hmap_search(&hmap, strhash(p, e.data), tcmp, data);
            if (!a) continue;
            u32 b = hmap_search(&hmap, strhash(e.size - p, e.data + p), tcmp, data);
            if (!b) continue;

        






fail:
    free( ibuf_drop(&ib) );
    if (hm) hmap_drop(hm);
    free(res);
    ctokenizer_drop(&t);
    return 0;
}


#if 0
static int tmap_init(tmap_s * tm, u32 n_slots) {
    u32 mask;
    for (mask = 1; 2 * mask < n_slots; mask = 2 * mask + 1);
    u32 * tpos = calloc(mask + 1, sizeof *tpos);
    if (ERR_ON(!tpos, "malloc"))
        return -1;
    tm->mask = mask;
    tm->tpos = tpos;
    return 0;
}

static void tmap_insert(tmap_s * tm, u32 size, char data[size]) {
    u32 slot = strhash(size, data);
    for (u32 step = 1; tmap[slot & mask]; slot += step, step += 2);
    tm.slot[slot & mask] = ;
}

static u32 tmap_search(tmap_s * tm, u32 size, char data[size]) {
    u32 slot = strhash(size, data);
    for (u32 step = 1; tmap[slot & mask]; slot += step, step += 2);
    return slot & mask;
}

static u32 next_power_of_2(u32 x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}
#endif
