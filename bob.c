// The Builder
#include <stdlib.h>

// TODO make alternative

// read in config file that tells us the compiler & args, others

// read in cache file to determine what is already built

// compare file contents or write date to determine what we need to rebuild
// (compared to cache file information)

int main(const int argc, const char** argv) {
    int ret = system("gcc -g -Wall -Wextra -o Main src/*.c -lSDL3 -lglslang -lSPIRV -lglslang-default-resource-limits");
    if (ret != 0) {
	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
