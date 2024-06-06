#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 modelMatrix;

// Simple vertex shader, transforming the raw position by a model matrix
// Part of the pointlight shadow cube computation
void main()
{
    gl_Position = modelMatrix * vec4(position, 1.0);
}  