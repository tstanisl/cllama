#include "cgguf.h"
#include "cutils.h"

#include <assert.h>

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
        if (cgguf_strequal(kv.key, "tokenizer.ggml.tokens")) {
            printf("\ttype=%d\n", kv.val.arr.type);
            assert(kv.val.arr.type == CGGUF_TYPE_STRING);
            cgguf_val_u v;
            for (int i = 0; cgguf_read_val(&kv.val.arr, &v); ++i)
                printf("\t[%d] = '%.*s'\n", i, (int)v.str.size, v.str.data);
        }
    }

    cgguf_drop(ctx);
}
