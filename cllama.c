#include <stdio.h>
#include <stdlib.h>

#include "cgguf.h"
#include "cutils.h"

int main(int argc, char * argv[argc + 1]) {
    if (ERR_ON(argc != 2, "Missing path to GGUF model"))
        return -1;
    auto ctx = cgguf_open(argv[1]);
    ASSERT(ctx);
}
