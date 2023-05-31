#version 450

layout (binding = 1) uniform sampler2D textureSampler;

layout (location = 0) in vec3 inColour;
layout (location = 1) in vec2 inTexturePosition;

layout (location = 0) out vec4 outColour;

void main() {
    vec4 texel = texture(textureSampler, inTexturePosition);
    //outColour = texel.r * vec4(inColour, 1.0);
    outColour = texel.r * vec4(1.0, 1.0, 1.0, 1.0);
    //outColour = vec4(inTexturePosition, 0.0, 0.0);
}
