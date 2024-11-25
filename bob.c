// The Builder
#include <stdlib.h>

// TODO make alternative

// read in config file that tells us the compiler & args, others

// read in cache file to determine what is already built

// compare file contents or write date to determine what we need to rebuild
// (compared to cache file information)

int main(const int argc, const char** argv) {
    system("gcc -Wall -Wextra -o Main src/*.c");
    return 0;
}
