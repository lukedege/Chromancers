#version 430 core
layout (location = 0) in vec3 position;
layout (location = 2) in vec2 tex_coords;

out vec2 TexCoords;

void main()
{
    TexCoords = tex_coords;
    gl_Position = vec4(position, 1.0); 
}