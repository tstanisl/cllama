#pragma once

#include <stddef.h>

typedef struct ctokenizer * ctokenizer_h;

typedef struct {
    const char * data;
    size_t       size;
} ctokenizer_entry_s;

typedef enum {
    CTOKENIZER_TYPE_GPT2 = 0,
    CTOKENIZER_TYPE_COUNT_,
} ctokenizer_type_e;

ctokenizer_h ctokenizer_init(
    ctokenizer_type_e type,
    int n_tokens,
    ctokenizer_entry_s get(void*, int), void * priv
);

void ctokenizer_drop(ctokenizer_h);
int ctokenizer_encode(ctokenizer_h, int len, const char str[len], int tokens[len]);

