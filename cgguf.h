#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct cgguf * cgguf_h;

typedef enum {
    CGGUF_DFMT_F32     = 0,
    CGGUF_DFMT_F16     = 1,
    CGGUF_DFMT_Q4_0    = 2,
    CGGUF_DFMT_Q4_1    = 3,
    // CGGUF_DFMT_Q4_2 = 4, support has been removed
    // CGGUF_DFMT_Q4_3 = 5, support has been removed
    CGGUF_DFMT_Q5_0    = 6,
    CGGUF_DFMT_Q5_1    = 7,
    CGGUF_DFMT_Q8_0    = 8,
    CGGUF_DFMT_Q8_1    = 9,
    CGGUF_DFMT_Q2_K    = 10,
    CGGUF_DFMT_Q3_K    = 11,
    CGGUF_DFMT_Q4_K    = 12,
    CGGUF_DFMT_Q5_K    = 13,
    CGGUF_DFMT_Q6_K    = 14,
    CGGUF_DFMT_Q8_K    = 15,
    CGGUF_DFMT_IQ2_XXS = 16,
    CGGUF_DFMT_IQ2_XS  = 17,
    CGGUF_DFMT_IQ3_XXS = 18,
    CGGUF_DFMT_IQ1_S   = 19,
    CGGUF_DFMT_IQ4_NL  = 20,
    CGGUF_DFMT_IQ3_S   = 21,
    CGGUF_DFMT_IQ2_S   = 22,
    CGGUF_DFMT_IQ4_XS  = 23,
    CGGUF_DFMT_I8      = 24,
    CGGUF_DFMT_I16     = 25,
    CGGUF_DFMT_I32     = 26,
    CGGUF_DFMT_I64     = 27,
    CGGUF_DFMT_F64     = 28,
    CGGUF_DFMT_IQ1_M   = 29,
    CGGUF_DFMT_BF16    = 30,
    // CGGUF_DFMT_Q4_0_4_4 = 31, support has been removed from gguf files
    // CGGUF_DFMT_Q4_0_4_8 = 32,
    // CGGUF_DFMT_Q4_0_8_8 = 33,
    CGGUF_DFMT_TQ1_0   = 34,
    CGGUF_DFMT_TQ2_0   = 35,
    // CGGUF_DFMT_IQ4_NL_4_4 = 36,
    // CGGUF_DFMT_IQ4_NL_4_8 = 37,
    // CGGUF_DFMT_IQ4_NL_8_8 = 38,
    CGGUF_DFMT_MXFP4   = 39, // MXFP4 (1 block)
    CGGUF_DFMT_COUNT   = 40,
} cgguf_type_e;

typedef enum {
    CGGUF_TYPE_UINT8 = 0,
    CGGUF_TYPE_INT8 = 1,
    CGGUF_TYPE_UINT16 = 2,
    CGGUF_TYPE_INT16 = 3,
    CGGUF_TYPE_UINT32 = 4,
    CGGUF_TYPE_INT32 = 5,
    CGGUF_TYPE_FLOAT32 = 6,
    CGGUF_TYPE_BOOL = 7,
    CGGUF_TYPE_STRING = 8,
    CGGUF_TYPE_ARRAY = 9,
    CGGUF_TYPE_UINT64 = 10,
    CGGUF_TYPE_INT64 = 11,
    CGGUF_TYPE_FLOAT64 = 12,
    // Used to share tensor and key-val interfaces
    CGGUF_TYPE_TENSOR = 128,
} cgguf_value_type_e;

enum { CGGUF_MAX_DIMS = 4 };

typedef struct {
    uint64_t len;
    // The string as a UTF-8 non-null-terminated string.
    const char * str;
} cgguf_str_s;

typedef struct {
    cgguf_value_type_e type;
    uint64_t len;
} cgguf_array_s;

typedef struct {
    cgguf_str_s  name;
    int64_t      dims[CGGUF_MAX_DIMS];
    cgguf_dfmt_e dfmt;
    const void * data;
} cgguf_tensor_s;

typedef union {
    cgguf_type_e type;
    union {
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
        cgguf_array_s array;
        cgguf_tensor_s tensor;
    };
} cgguf_val_u;

typedef struct cgguf_itr_s {
    uint64_t left;
    uint64_t priv[3];
};

cgguf_h     cgguf_open(const char *fname);
void        cgguf_drop(cgguf_h);
cgguf_itr_s cgguf_iter(cgguf_h);
_Bool       cgguf_peek(cgguf_itr_s *, cgguf_str_s*, cgguf_val_s*);
_Bool       cgguf_fine(cgguf_itr_s *);
void        cgguf_cont(cgguf_itr_s *);
void        cgguf_nest(cgguf_itr_s *);

// compare cgguf_str_s with 0-terminated string
int cgguf_strequal(const cgguf_str_s *, const char *);

#if 0

typedef struct {
    uint64_t n_keyvals;
    uint64_t n_tensors;
} cgguf_params_s;

cgguf_params_s cgguf_params_get(cgguf_h);
cgguf_keyval_s cgguf_keyval_get(cgguf_h, uint64_t);
cgguf_tensor_s cgguf_tensor_get(cgguf_h, uint64_t);

const ccguf_val_u * cgguf_arr_iter(cgguf_arr_s);
cgguf_val_u cgguf_arrval_next(cgguf_itr_s *);

void cgguf_keyval_init(cgguf_h, cgguf_keyval_s * kv);
void cgguf_keyval_cont(cgguf_h, cgguf_keyval_s * kv);
void cgguf_keyval_read(cgguf_h, cgguf_keyval_s * kv);

#define CGGUF_FOREACH_KEYVAL(_h, _it) \
    for (cgguf_keyval_init(_h, &_it); \
         _it.key.str;                 \
         cgguf_keyval_cont(_h, &_it)) \
// Tensor helpers


cgguf_tensor_s cgguf_tensor_start(cgguf_h);
cgguf_tensor_s cgguf_tensor_next(cgguf_h, cgguf_tensor_s kv);

#define CGGUF_FOREACH_TENSOR(_h, _it) \
    for (cgguf_tensor_s _it = cgguf_tensor_start(_h); \
         _it.name;                                    \
         _it = cgguf_tensor_next(_h, _it))            \

auto h = cgguf_open(...);
cgguf_str_s key;
cgguf_val_s val;
for (auto it = cgguf_iter(h); cgguf_peek(&it, &key, &val); cgguf_cont(&it)) {
    if (val.type == CGGUF_TYPE_ARRAY) {
        for (cgguf_nest(&it); cgguf_peek(&it, 0, &val); cgguf_cont(&it)) 
    }
}


#endif

