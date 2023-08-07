#version 430 core

layout (location = 0) in vec3 pos;

out gl_PerVertex { vec4 gl_Position; };

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
   // note that we read the multiplication from right to left
   gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(pos, 1.0);
}