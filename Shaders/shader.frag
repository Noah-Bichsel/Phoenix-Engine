#version 450

// Final output color (must also have location)
// location out can only connect to a locaion in so location 0 is ok here
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}