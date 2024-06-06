#version 430 core

layout (location = 0) in vec3 position; // vertex info

out gl_PerVertex { vec4 gl_Position; }; // output shader variable

// Basic vertex shader, relays the vertex position as is
void main()
{
   gl_Position = vec4(position.x, position.y, position.z, 1.0f);
}