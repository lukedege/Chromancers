#version 430 core
layout (location = 0) in vec3 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

// Simple vertex shader, transforming raw position into world then light space
void main()
{
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(position, 1.0);
}  