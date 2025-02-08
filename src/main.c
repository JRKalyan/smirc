#include <stdio.h>
#include "context.h"
#include "stf.h"
#include "shader.h"

// REMOVE
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Context context;
    if (!context_init(&context)) {
        puts(stfa_to_ascii[STFA_OUDENTHORPE]);
        return EXIT_FAILURE;
    }
    else {
        for (int i = 0; i < 3; i++) {
            puts(stfa_to_ascii[STFA_MONOTHORPE]);
            context_sleep(&context, 300);
        }
        context_destroy(&context);
    }

    return EXIT_SUCCESS;
}
