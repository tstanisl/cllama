#include "cgguf.h"
#include "cutils.h"

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    cgguf_h ctx = cgguf_open(argv[1]);
    ASSERT(ctx);

#if 0
    CGGUF_FOREACH_KEYVAL(ctx, kv) {
        printf("key[%.*s] -> %p\n", (int)kv.key->len, kv.key->str, (void*)kv.val);
    }
#endif

    cgguf_drop(ctx);
}
