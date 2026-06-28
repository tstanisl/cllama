#include "cgguf.h"
#include "cutils.h"

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    cgguf_h ctx = cgguf_open(argv[1]);
    ASSERT(ctx);

/*
    cgguf_keyval_s kv;
    CGGUF_FOREACH_KEYVAL(ctx, kv) {
        printf("key[%.*s]\n", (int)kv.key.len, kv.key.str);
    }
*/

    cgguf_str_s key;
    for (auto it = cgguf_iter(ctx); cgguf_peek(&it, &key, 0); cgguf_cont(&it)) {
        printf("key[%.*s]\n", (int)key.len, key.str);
    }

    cgguf_drop(ctx);
}
