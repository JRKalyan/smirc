#glslangValidator -V "shaders/shader.vert" -o "src/vert.spv" && \
#glslangValidator -V "shaders/shader.frag" -o "src/frag.spv" && \


# PREVIOUS: Compile and run Bob
#gcc -g -o bob "bob.c" && ./bob && ./Main
#
#

# NEW TEST: directly call gcc
BUILD_CMD="gcc -g -Wall -Wextra -o Main src/*.c $(guile-config compile) -lSDL3 -lglslang -lSPIRV -lglslang-default-resource-limits $(guile-config link)"

$BUILD_CMD && ./Main

