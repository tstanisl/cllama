#include "ctokenizer.h"

#include "cutils.h"

typedef struct {
    u32 a, b, t;
} merge_s;

struct ctokenizer {
    u32 n_tokens;
    char    * str;
    u32     * off;
    merge_s * mrg;
};

static u64 strhash(u32 len, const char str[len]) {
    // FNV hash
    u64 hash = 0xcbf29ce484222325;
    for (u32 i = 0; i < len; ++i) {
        hash *= 0x00000100000001b3;
        hash ^= str[i];
    }
    return hash;
}

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
