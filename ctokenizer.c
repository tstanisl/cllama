#include "ctokenizer.h"

#include "cutils.h"

typedef struct {
    u8   len;
    char str[];
} token_s;

struct ctokenizer {
    char * data;
    u32  * tmap;
    u32    tmask;
};

typedef struct {
    u32   mask;
    u32 * data;
} hmap_s;

static int hmap_init(hmap_s * hm, u32 n_slots) {
    u32 mask;
    for (mask = 1; 2 * mask < n_slots; mask = 2 * mask + 1);
    u32 * data = calloc(mask + 1, sizeof *data);
    if (ERR_ON(!data, "malloc"))
        return -1;
    hm->mask = mask;
    hm->data = data;
    return 0;
}

static void hmap_drop(hmap_s * hm) {
    free(hm->data);
    *hm = (hmap_s) { 0 };
}

static void hmap_insert(hmap_s * hm, u32 hash, u32 value) {
    assert(value);
    u32 * data = hm->data;
    u32   mask = hm->mask;
    for (u32 step = 1; 1; hash += step, step += 2)
        if (!data[hash & mask]) {
            data[hash & mask] = value;
            return;
        }
}

static u32 hmap_search(
    hmap_s * hm, u32 hash,
    bool cmp(u32, const void*), const void *priv
) {
    u32 * data = hm->data;
    u32   mask = hm->mask;
    for (u32 step = 1; 1; hash += step, step += 2) {
        u32 value = data[hash & mask];
        if (!value) return 0;
        if (cmp(value, priv)) return value;
    }
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

typedef struct {
    u32 a, b, t;
} merge_s;

ctokenizer_h ctokenizer_init(
    ctokenizer_type_e type,
    int n_tokens,
    ctokenizer_entry_s get(void*, int), void * priv
) {
    char * data = 0;
    hmap_s hmap = { 0 };
    size_t size = 0;
    for (int i = 0; i < n_tokens; ++i)
        size += sizeof(token_s) + get(priv, i).size;

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
        hmap_insert(&hm, strhash(e.size, e.data), head + 1 - data);
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
    free(tmap);
    free(data);
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
