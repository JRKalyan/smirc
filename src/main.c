#include <stdio.h>
#include "stf.h"
#include "vk.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (load_vulkan())
    {
        // TODO
        puts(stfa_to_ascii[STFA_MONOTHORPE]);

        unload_vulkan();
    }
    else {
        puts(stfa_to_ascii[STFA_OUDENTHORPE]);
    }

    return 0;
}
