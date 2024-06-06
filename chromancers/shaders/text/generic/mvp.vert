#version 430 core

layout (location = 0) in vec3 pos;

out gl_PerVertex { vec4 gl_Position; };

uniform mat4 modelMatrix      = mat4(1);
uniform mat4 viewMatrix       = mat4(1);
uniform mat4 projectionMatrix = mat4(1);

// Simple vertex shader, transforms the vertex position by using the MVP matrices
void main()
{
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(pos, 1.0);
}