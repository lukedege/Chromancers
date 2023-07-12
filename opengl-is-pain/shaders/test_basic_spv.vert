#version 410 core

#include "types.glsl"
#include "constants.glsl"

layout (location = 0) in vec3 position;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
   gl_Position = vec4(position.x, position.y, position.z, 1.0f);
}