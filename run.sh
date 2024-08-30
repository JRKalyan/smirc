#glslangValidator -V "shaders/shader.vert" -o "src/vert.spv" && \
#glslangValidator -V "shaders/shader.frag" -o "src/frag.spv" && \
gcc -g -o bob "bob.c" && ./bob && ./Main
