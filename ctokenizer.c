#include "ctokenizer.h"

#include "cutils.h"
#include "hmap.h"
#include "ibuf.h"

#include <assert.h>
#include <limits.h>

typedef struct {
    u32 a, b, t;
} merge_s;

typedef struct ctokenizer {
    u32 n_tokens;
    char    * data;
    u32     * start;
    merge_s * merge;
    hmap_h    hm;
    int       direct[256];
    int       n_unsafe;
    char      unsafe[256];
} ctokenizer_s;

static inline const char * tokstr(ctokenizer_s * t, int tok) {
    return t->data + t->start[tok];
}

static inline size_t toklen(ctokenizer_s * t, int tok) {
    return t->start[tok + 1] - t->start[tok];
}

static u32 strhash(u32 len, const char str[len]) {
    // FNV hash
    u32 hash = 0x811c9dc5;
    for (u32 i = 0; i < len; ++i) {
        hash *= 0x01000193;
        hash ^= str[i];
    }
    hash ^= hash >> 16; // avalanche (Murmur3 fmix32)
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

static uint32_t hash_token(uint32_t idx, const void* t_) {
    const ctokenizer_s * t = t_;
    u32 * s = t->start;
    //printf("'%.*s'", s[idx + 1] - s[idx], t->data + s[idx]);
    return strhash(s[idx + 1] - s[idx], t->data + s[idx]);
}

typedef struct {
    ctokenizer_entry_s entry;
    const char * data;
    u32 * start;
} test_token_ctx_s;

static _Bool test_token(uint32_t idx, const void* ctx_) {
    test_token_ctx_s ctx = *(const test_token_ctx_s*)ctx_;
    u32 a_size = ctx.start[idx + 1] - ctx.start[idx];
    const char * a_data = ctx.data + ctx.start[idx];
    u32 b_size = ctx.entry.size;
    const char * b_data = ctx.entry.data;
#if 0
    printf("\ttest_token %.*s vs (%u) %.*s\n",
           (int)a_size, a_data, idx, (int)b_size, b_data);
#endif
    if (a_size != b_size)
        return 0;
    return memcmp(a_data, b_data, a_size) == 0;
}

static u32 find_token(ctokenizer_h t, hmap_h hm, u32 size, char * data) {
    //printf("find(%u,%.*s)", size, (int)size, data);
    test_token_ctx_s ctx = {
        .entry = { data, size },
        .data = t->data, .start = t->start
    };
    return hmap_search(hm, strhash(size, data), test_token, &ctx);
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

static uint32_t hash_merge(uint32_t idx, const void* t_) {
    const ctokenizer_s * t = t_;
    merge_s m = t->merge[idx];
    return hash2x32(m.a, m.b);
}

typedef struct {
    const merge_s * merge;
    u32 a, b;
} test_merge_ctx_s;

static _Bool test_merge(uint32_t idx, const void* ctx_) {
    const test_merge_ctx_s * ctx = ctx_;
    merge_s m = ctx->merge[idx];
    return m.a == ctx->a && m.b == ctx->b;
}

ctokenizer_h ctokenizer_init(
    ctokenizer_type_e type,
    int n_tokens,
    ctokenizer_entry_s next_token(void*), void * next_token_ctx
) {
    assert(type == CTOKENIZER_TYPE_GPT2);

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

#if 0
    for (int i = 0; i < n_tokens; ++i) {
        printf("%.*s\n", (int)(t.start[i + 1] - t.start[i]),
                         t.data + t.start[i]);
    }
#endif

    hm = hmap_init(n_tokens, hash_token, &t);
    if (ERR_ON(!hm, "hmap_init"))
        goto fail;

    t.n_unsafe = 0;
    //int n_safe = 0;
    for (int i = 0; i < 256; ++i) {
        char txt[3] = { 0 };
        // check if byte is unsafe
        if ((i <= 32) || (127 <= i && i <= 160) || i == 173) {
            int unicode = 0x100 + t.n_unsafe;
            txt[0] = 0xc0 + (unicode >> 6);
            txt[1] = 0x80 + (unicode & 63);
            //printf("n_unsafe=%u\n", t.n_unsafe);
            assert(t.n_unsafe <= (int)sizeof t.unsafe);
            t.unsafe[t.n_unsafe++] = i;
        } else if (i >= 0x80) {
            int unicode = i;
            txt[0] = 0xc0 + (unicode >> 6);
            txt[1] = 0x80 + (unicode & 63);
        } else {
            txt[0] = i;
        }
        //printf("\ttok[%3d] = %s (%d,%d)", i, txt, (u8)txt[0], (u8)txt[1]);
        u32 tok = find_token(&t, hm, strlen(txt), txt);
        if (ERR_ON(tok == HMAP_NONE, "no token for byte %d", i))
            break;
            //goto fail;
        t.direct[i] = tok;
        //printf(" -> %d: %.*s\n", (int)tok, t.start[tok + 1] - t.start[tok], t.data + t.start[tok]);
    }

    u32 n_merges = 0;
    for (int i = 0; i < n_tokens; ++i) {
        char * data = t.data + t.start[i];
        u32    size = t.start[i + 1] - t.start[i];
        #if 0
        printf("split %5d: %.*s:\n", i, (int)size, data);
        // sanity check
        int idx = find_token(&t, hm, size, data);
        printf(" idx=%d", idx);
        fflush(stdout);
        assert(idx == i);
        #endif
        for (u32 p = 1; p < size; ++p) {
            u32 a = find_token(&t, hm, p, data);
            if (a == HMAP_NONE) continue;
            u32 b = find_token(&t, hm, size - p, data + p);
            if (b == HMAP_NONE) continue;
            merge_s * m = ibuf_grow(&ib, sizeof *m);
            if (ERR_ON(!m, "ibuf_grow"))
                goto fail;
            *m = (merge_s) { a, b, i };
            /*
            printf("\t(%u,%u) '%.*s %.*s' -> %.*s\n", a, b,
                   (int)p, data, (int)(size-p), data+p,
                   (int)size, data);
            */
            ++n_merges;
        }
        //puts("");
    }
    //printf("n_merges=%u\n", n_merges);
    t.merge = ibuf_drop(&ib);

    // mapping from string to token is no longer needed
    hmap_drop(hm);
    hm = 0;
#if 1
    // reuse hm for mapping merges
    t.hm = hmap_init(n_merges, hash_merge, &t);
    if (ERR_ON(!t.hm, "hmap_init"))
        goto fail;
#endif

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
    hmap_drop(t->hm);
    free(t->data);
    free(t->start);
    free(t->merge);
    free(t);
}

size_t ctokenizer_encode(ctokenizer_h t,
    size_t len, const char str[len],
    int tokens[len]
) {
    //printf("str='%.*s'\n", (int)len, str);
    for (size_t i = 0; i < len; ++i)
        tokens[i] = t->direct[(u8)str[i]];
    for (;;) {
        #if 1
        for (size_t i = 0; i < len; ++i) {
            int tok = tokens[i];
            size_t tsize = t->start[tok + 1] - t->start[tok];
            char * tdata = t->data + t->start[tok];
            printf(" %d:%.*s", tok, (int)tsize, tdata);
        }
        puts("");
        #endif
        size_t best_pos = len;
        u32 best_token = HMAP_NONE;
        for (size_t i = 0; i + 1 < len; ++i) {
            u32 a = tokens[i];
            u32 b = tokens[i + 1];
            test_merge_ctx_s ctx = { t->merge, a, b };
            u32 midx = hmap_search(t->hm, hash2x32(a, b), test_merge, &ctx);
            if (midx == HMAP_NONE) continue;
            merge_s m = t->merge[midx];
            if (m.t < best_token) {
                best_token = m.t;
                best_pos = i;
            }
        }
        if (best_token == HMAP_NONE)
            break;
        tokens[best_pos] = (int)best_token;
        for (size_t j = best_pos + 2; j < len; ++j)
            tokens[j - 1] = tokens[j];
        --len;
    }
    return len;
}

size_t ctokenizer_decode(
    ctokenizer_h t,
    size_t len, const int tokens[len],
    size_t max_chars, char buf[restrict max_chars]
) {
    //size_t left = max_chars;
    size_t p = 0;
    for (size_t i = 0; i < len; ++i) {
        int tok = tokens[i];
        size_t tlen = toklen(t, tok);
        if (tlen >= max_chars - p)
            break;
        memcpy(buf + p, tokstr(t, tok), tlen);
        p += tlen;
    }
    // replace unicode in dangerous range with bytes
    size_t j = 0;
    for (size_t i = 0; i < p; ++i) {
        int c0 = (u8)buf[i];
        //printf("%c c0=%d\n", c0, c0);
        if (0xc0 <= c0 && c0 < 0xe0 && i + 1 < p) {
            int c1 = (u8)buf[i + 1];
            //printf("\t%c c1=%d\n", c1, c1);
            if (0x80 <= c1 && c1 < 0xc0) {
                int u = ((c0 & 0x1f) << 6) + (c1 & 0x3f);
                //printf("\tu=%d\n", u);
                if (0x100 <= u && u < 0x100 + t->n_unsafe) {
                    c0 = t->unsafe[u - 0x100];
                } else {
                    c0 = u;
                }
                //printf("\tc0=%d, %c\n", c0, c0);
                ++i;
            }
        }
        buf[j++] = c0;
    }
    return j;
}

