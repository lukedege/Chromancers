#version 460 core

// output variable for the fragment shader. Usually, it is the final color of the fragment
layout (location = 0) out vec4 color;

void main()
{
	color = vec4(1.0f, 1.0f, 0.0f, 0.1f);
}