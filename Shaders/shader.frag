#version 450

layout(location = 0) in vec3 fragCol;

// Final output color (must also have location)
// location out can only connect to a locaion in so location 0 is ok here
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragCol, 1.0);
}