#version 450

layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragTex;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

// Final output color (must also have location)
// location out can only connect to a locaion in so location 0 is ok here
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(textureSampler, fragTex);
}