#version 430 core

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 tex_coords;

out vec2 TexCoords;

// Simple vertex shader, relaying raw 2D position and tex coordinates
// useful for full-screen quads
void main()
{
   TexCoords = tex_coords;
   gl_Position = vec4(position.x, position.y, 0, 1.0f);
}