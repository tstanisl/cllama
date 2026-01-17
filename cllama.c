#include "cgguf.h"
#include "cutils.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    auto ctx = cgguf_open(argv[1]);
    ASSERT(ctx);
}
