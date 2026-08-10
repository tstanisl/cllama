#include "hmap.h"
#include "cutils.h"

typedef struct hmap {
    u32 mask;
    u32 data[];
} hmap_s;

hmap_h hmap_init(uint32_t size, uint32_t hash_cb(uint32_t, const void*), const void *ctx) {
    u32 mask;
    for (mask = 1; 3 * size / 2 >= mask; mask = 2 * mask + 1);
    //printf("size=%u mask=%u\n", size, mask);
    hmap_s * hm = calloc(1, sizeof *hm + (mask + 1) * sizeof hm->data[0]);
    if (ERR_ON(!hm, "malloc"))
        return 0;
    hm->mask = mask;
    u32 * data = hm->data;
    for (u32 i = 0; i < size; ++i) {
        //printf("  insert %u:", i);
        u32 pos = hash_cb(i, ctx) & mask;
        for (u32 step = 1; data[pos]; pos = (pos + step++) & mask)
            ;
            //printf(" %u", pos);
        //printf(" %u\n", pos);
        data[pos] = ~i;
    }
    return hm;
}

void hmap_drop(hmap_h hm) {
    free(hm);
}

uint32_t hmap_search(hmap_h hm, uint32_t hash, _Bool test(uint32_t, const void*), const void * ctx) {
    u32 mask = hm->mask;
    u32 * data = hm->data;
    u32 pos = hash & mask;
    for (u32 step = 1; data[pos]; pos = (pos + step++) & mask) {
        u32 idx = ~data[pos];
        if (test(idx, ctx))
            return idx;
    }
    return -1;
}

#if 0
    u32 * data = calloc(1, sizeof *data);
    
static hmap_s * hmap_init(hmap_s * hm, u32 max_slots) {
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
#endif

