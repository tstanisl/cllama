#include "cgguf.h"
#include "cutils.h"

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    cgguf_h ctx = cgguf_open(argv[1]);
    ASSERT(ctx);

    for (u64 i = 0; i < ctx->n_keyvals; ++i) {
        cgguf_keyval_s kv = ctx->keyvals[i];
        printf("key[%.*s]\n", (int)kv.key.size, kv.key.data);
    }

    cgguf_drop(ctx);
}
