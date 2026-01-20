#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CGGUF_TYPE_F32     = 0,
    CGGUF_TYPE_F16     = 1,
    CGGUF_TYPE_Q4_0    = 2,
    CGGUF_TYPE_Q4_1    = 3,
    // CGGUF_TYPE_Q4_2 = 4, support has been removed
    // CGGUF_TYPE_Q4_3 = 5, support has been removed
    CGGUF_TYPE_Q5_0    = 6,
    CGGUF_TYPE_Q5_1    = 7,
    CGGUF_TYPE_Q8_0    = 8,
    CGGUF_TYPE_Q8_1    = 9,
    CGGUF_TYPE_Q2_K    = 10,
    CGGUF_TYPE_Q3_K    = 11,
    CGGUF_TYPE_Q4_K    = 12,
    CGGUF_TYPE_Q5_K    = 13,
    CGGUF_TYPE_Q6_K    = 14,
    CGGUF_TYPE_Q8_K    = 15,
    CGGUF_TYPE_IQ2_XXS = 16,
    CGGUF_TYPE_IQ2_XS  = 17,
    CGGUF_TYPE_IQ3_XXS = 18,
    CGGUF_TYPE_IQ1_S   = 19,
    CGGUF_TYPE_IQ4_NL  = 20,
    CGGUF_TYPE_IQ3_S   = 21,
    CGGUF_TYPE_IQ2_S   = 22,
    CGGUF_TYPE_IQ4_XS  = 23,
    CGGUF_TYPE_I8      = 24,
    CGGUF_TYPE_I16     = 25,
    CGGUF_TYPE_I32     = 26,
    CGGUF_TYPE_I64     = 27,
    CGGUF_TYPE_F64     = 28,
    CGGUF_TYPE_IQ1_M   = 29,
    CGGUF_TYPE_BF16    = 30,
    // CGGUF_TYPE_Q4_0_4_4 = 31, support has been removed from gguf files
    // CGGUF_TYPE_Q4_0_4_8 = 32,
    // CGGUF_TYPE_Q4_0_8_8 = 33,
    CGGUF_TYPE_TQ1_0   = 34,
    CGGUF_TYPE_TQ2_0   = 35,
    // CGGUF_TYPE_IQ4_NL_4_4 = 36,
    // CGGUF_TYPE_IQ4_NL_4_8 = 37,
    // CGGUF_TYPE_IQ4_NL_8_8 = 38,
    CGGUF_TYPE_MXFP4   = 39, // MXFP4 (1 block)
    CGGUF_TYPE_COUNT   = 40,
} cgguf_type_e;

typedef enum {
    CGGUF_VALUE_TYPE_UINT8 = 0,
    CGGUF_VALUE_TYPE_INT8 = 1,
    CGGUF_VALUE_TYPE_UINT16 = 2,
    CGGUF_VALUE_TYPE_INT16 = 3,
    CGGUF_VALUE_TYPE_UINT32 = 4,
    CGGUF_VALUE_TYPE_INT32 = 5,
    CGGUF_VALUE_TYPE_FLOAT32 = 6,
    CGGUF_VALUE_TYPE_BOOL = 7,
    CGGUF_VALUE_TYPE_STRING = 8,
    CGGUF_VALUE_TYPE_ARRAY = 9,
    CGGUF_VALUE_TYPE_UINT64 = 10,
    CGGUF_VALUE_TYPE_INT64 = 11,
    CGGUF_VALUE_TYPE_FLOAT64 = 12,
} cgguf_value_type_e;

enum { CGGUF_MAX_DIMS = 4 };

typedef struct {
    uint64_t len;
    // The string as a UTF-8 non-null-terminated string.
    char str[/* len */];
} cgguf_str_s;

typedef struct {
    cgguf_value_type_e type;
    uint64_t len;
} cgguf_arr_s;

typedef union {
    uint8_t  u8;
    int8_t   i8;
    uint16_t u16;
    int16_t  i16;
    uint32_t u32;
    int32_t  i32;
    float    f32;
    uint64_t u64;
    int64_t  i64;
    double   f64;
    bool     b8;
    cgguf_str_s str;
    cgguf_arr_s arr;
} cgguf_val_u;


typedef struct cgguf * cgguf_h;

cgguf_h cgguf_open(const char *fname);
void cgguf_drop(cgguf_h);

// Key-Value helpers
typedef struct {
    const cgguf_str_s * key;
    const cgguf_val_u * val;
} cgguf_keyval_s;

int cgguf_strequal(const cgguf_str_s *, const char *);

cgguf_keyval_s cgguf_keyval_start(cgguf_h);
cgguf_keyval_s cgguf_keyval_next(cgguf_h, cgguf_keyval_s kv);

#define CGGUF_FOREACH_KEYVAL(_h, _it) \
    for (cgguf_keyval_s _it = cgguf_keyval_start(_h); \
         _it.key;                                     \
         _it = cgguf_keyval_next(_h, _it))            \

// Tensor helpers

typedef struct {
    const cgguf_str_s * name;
    void * data;
    int64_t dims[CGGUF_MAX_DIMS];
    cgguf_value_type_e type;
} cgguf_tensor_s;

cgguf_tensor_s cgguf_tensor_start(cgguf_h);
cgguf_tensor_s cgguf_tensor_next(cgguf_h, cgguf_tensor_s kv);

#define CGGUF_FOREACH_TENSOR(_h, _it) \
    for (cgguf_tensor_s _it = cgguf_tensor_start(_h); \
         _it.name;                                    \
         _it = cgguf_tensor_next(_h, _it))            \

