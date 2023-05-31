glslangValidator -V "shaders/shader.vert" -o "src/vert.spv" && \
glslangValidator -V "shaders/shader.frag" -o "src/frag.spv" && \
zig build && \
zig-out/bin/smirc
