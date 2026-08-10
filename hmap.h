#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct hmap * hmap_h;

hmap_h hmap_init(uint32_t size, uint32_t hash_cb(uint32_t, const void*), const void *ctx);
void hmap_drop(hmap_h);
#define HMAP_NONE ((uint32_t)-1)
uint32_t hmap_search(hmap_h, uint32_t hash, _Bool test(uint32_t, const void*), const void * ctx);
