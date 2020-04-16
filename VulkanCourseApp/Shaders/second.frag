#version 450 // Use GLSL 4.5

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor; // Color output from subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth; // Depth output from subpass 1

layout(location = 0) out vec4 color;

void main()
{
	int xHalf = 1366 / 2;
	if (gl_FragCoord.x > xHalf)
	{
		float lowerBound = 0.98;
		float upperBound = 1.0;

		float depth = subpassLoad(inputDepth).r;
		float depthColorScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));

		color = vec4(depthColorScaled, depthColorScaled, depthColorScaled, 1.0);
	}
	else
	{
		color = subpassLoad(inputColor).rgba;
	}
}
