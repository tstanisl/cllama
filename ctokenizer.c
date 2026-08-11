#include "ctokenizer.h"

#include "cutils.h"
#include "hmap.h"
#include "ibuf.h"

typedef struct {
    u32 a, b, t;
} merge_s;

typedef struct ctokenizer {
    u32 n_tokens;
    char    * data;
    u32     * start;
    merge_s * merge;
} ctokenizer_s;

static u32 strhash(u32 len, const char str[len]) {
    // FNV hash
    u32 hash = 0x811c9dc5;
    for (u32 i = 0; i < len; ++i) {
        hash *= 0x01000193;
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
    ctokenizer_entry_s next_token(void*, int), void * next_token_ctx
) {
    ctokenizer_s t = {
        .start = calloc(n_tokens + 1, sizeof *t.start),
    };
    ctokenizer_s * res = malloc(sizeof *res);
    hmap_h hm = 0;
    ibuf_s ib = IBUF_INIT;

    if (ERR_ON(!t.start || !res, "malloc"))
        goto fail;

    u32 head = 0;
    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = next_token(next_token_ctx);
        char * data = ibuf_grow(&ib, e.size);
        if (ERR_ON(!data, "ibuf_grow"))
            goto fail;
        // todo: handle byte-tokens
        memcpy(data, e.data, e.size);
        t.start[i] = head;
        head += e.size;
    }
    t.start[n_tokens] = head; // sentinel
    t.data = ibuf_drop(&ib);

    hm = hmap_init(n_tokens, hash_token, &t);
    if (ERR_ON(!hm, "hmap_init"))
        goto fail;

    u32 n_merges = 0;
    for (int i = 0; i < n_tokens; ++i) {
        ctokenizer_entry_s e = get(priv, i);
        for (u32 p = 1; p + 1 < e.size; ++p) {
            u32 a = hmap_search(&hmap, strhash(p, e.data), tcmp, &t);
            if (a == HMAP_NONE) continue;
            u32 b = hmap_search(&hmap, strhash(e.size - p, e.data + p), tcmp, &t);
            if (b == HMAP_NONE) continue;
            merge_s * m = ibuf_grow(&ib, sizeof *m);
            if (ERR_ON(!m, "ibuf_grow"))
                goto fail;
            *m = (merge_s) { a, b, i };
            ++n_merges;
        }
    }
    t.merges = ibuf_drop(&ib);

    // mapping from string to token is no longer needed
    hmap_drop(hm);
    hm = 0;

    // reuse hm for mapping merges
    t.hm = hmap_init(n_merges, merge_hash, 0);
    if (ERR_ON(!t.hm, "hmap_init"))
        goto fail;

    *res = t;
    return res;

fail:
    free( ibuf_drop(&ib) );
    if (hm) hmap_drop(hm);
    free(res);
    ctokenizer_drop(&t);
    return 0;
}

void ctokenizer_drop(ctokenizer_h t) {
    hm_drop(t->hm);
    free(t->data);
    free(t->start);
    free(t->merge);
    *t = (ctokenizer_s) { 0 };
}

