#include "cgguf.h"
#include "ctokenizer.h"
#include "cutils.h"

#include <assert.h>
#include <inttypes.h>

ctokenizer_entry_s next_token(void * arr_) {
    cgguf_val_u v;
    cgguf_read_val(arr_, &v);
    return (ctokenizer_entry_s) { v.str.data, v.str.size };
}

void test_tokenizer(cgguf_keyval_s kv) {
    // TODO: add a few asserts
    cgguf_arr_s arr = kv.val.arr;
    ctokenizer_h t = ctokenizer_init(
        CTOKENIZER_TYPE_GPT2,
        arr.left,
        next_token,
        &arr
    );
    if (ERR_ON(!t))
        abort();

    char txt[] = "hello world";
    int tokens[sizeof txt];

    int len = ctokenizer_encode(&t, sizeof txt - 1, txt, tokens);
    printf("len=%d\n", len);

    ctokenizer_drop(t);
}

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    cgguf_h ctx = cgguf_open(argv[1]);
    ASSERT(ctx);

    for (u64 i = 0; i < ctx->n_keyvals; ++i) {
        cgguf_keyval_s kv = ctx->keyvals[i];
        printf("key[%.*s] type=%d\n",
               (int)kv.key.size, kv.key.data,
               (int)kv.type);
        if (kv.type == CGGUF_TYPE_STRING)
            printf("\t%.*s\n", (int)kv.val.str.size, kv.val.str.data);
        if (kv.type == CGGUF_TYPE_ARRAY)
            printf("\tsize=%" PRIu64 "\n", kv.val.arr.left);
        if (cgguf_strequal(kv.key, "tokenizer.ggml.tokens")) {
            test_tokenizer(kv);
        }
        #if 1
        static const char* arrays[] = {
            "general.languages",
            "general.tags",
            "tokenizer.ggml.tokens",
            //"tokenizer.ggml.merges",
            0
        };
        for (int j = 0; arrays[j]; ++j)
            if (cgguf_strequal(kv.key, arrays[j])) {
                printf("\ttype=%d\n", kv.val.arr.type);
                assert(kv.val.arr.type == CGGUF_TYPE_STRING);
                cgguf_val_u v;
                for (int i = 0; cgguf_read_val(&kv.val.arr, &v); ++i)
                    printf("\t[%d] = '%.*s'\n", i, (int)v.str.size, v.str.data);
            }
        #endif
    }
    for (u64 i = 0; i < ctx->n_tensors; ++i) {
        cgguf_tensor_s t = ctx->tensors[i];
        printf("tensor[%.*s] type=%d dims=",
               (int)t.name.size, t.name.data, (int)t.dfmt);
        for (int d = 0; d < CGGUF_MAX_DIMS; ++d)
            printf("%c%d", d ? ',' : '[', (int)t.dims[d]);
        puts("]");
    }

    cgguf_drop(ctx);
}
