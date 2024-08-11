#version 450

layout (binding = 0) uniform TestName {
    vec2 scale;
} transform;

layout (push_constant) uniform TestName2 {
    vec4 data;
} pushConstant;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexturePosition;

layout(location = 0) out vec3 outColour; 
layout(location = 1) out vec2 outTexturePosition;

void main() {
    //gl_Position = vec4(inPosition * transform.scale, 0.0, 1.0);
    vec2 proj = (transform.scale * (inPosition.xy * inPosition.z)) + pushConstant.data.xy;
    gl_Position = vec4(proj, 0, 1.0);
    outColour = inColour;
    //outTexturePosition = inTexturePosition * transform.scale;
    outTexturePosition = inTexturePosition;
}
