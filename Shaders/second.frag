#version 450

// color output from subpass 1
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
// depth output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 color;

void main()
{
    float lowerBound = 0.995f;
    float upperBound = 1.0f;

    vec4 colorLoaded = subpassLoad(inputColor).rgba;

    float depth = subpassLoad(inputDepth).r;
    float depthColorScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));

    color = vec4(colorLoaded.rgb * depthColorScaled, colorLoaded.a);
}