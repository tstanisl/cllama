#pragma once

#include <stddef.h>

typedef struct ctokenizer * ctokenizer_h;

typedef struct {
    const char * data;
    uint8_t      size;
} ctokenizer_entry_s;

typedef enum {
    CTOKENIZER_TYPE_GPT2 = 0,
    CTOKENIZER_TYPE_COUNT_,
} ctokenizer_type_e;

ctokenizer_h ctokenizer_init(
    ctokenizer_type_e type,
    int n_tokens,
    ctokenizer_entry_s next_token(void*), void * next_token_ctx
);

void ctokenizer_drop(ctokenizer_h);
size_t ctokenizer_encode(ctokenizer_h, size_t len, const char str[len], int tokens[len]);
size_t ctokenizer_decode(ctokenizer_h, size_t len, const int tokens[len], int max_chars, char buf[restrict max_chars]);

