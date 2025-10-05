#include <stdio.h>
#include <stdlib.h>

#include "cgguf.h"

#define ASSERT(...) \
    if (__VA_ARGS__); \
    else fprintf(stderr, "%s:%d: ASSERT: %s\n", \
                         __FILE__, __LINE__, \
                         # __VA_ARGS__), \
         abort()

int main(int argc, char * argv[]) {
    auto ctx = cgguf_open(argv[1]);
    ASSERT(ctx);
}
