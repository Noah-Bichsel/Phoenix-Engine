// Use GLSL 4.5
#version 450

// Output color for vertex (location is required)
layout(location = 0) out vec3 fragColor;

// Triangle vertex position (will put in to vetex buffer later!)
vec3 postions[3] = vec3[](
    vec3(0.0, -0.4, 0.0),
    vec3(0.4, 0.4, 0.0),
    vec3(-0.4, 0.4, 0.0)
);

// Triangle vertex colors
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    gl_Position = vec4(postions[gl_VertexIndex], 1.0);
    fragColor = colors[gl_VertexIndex];
}