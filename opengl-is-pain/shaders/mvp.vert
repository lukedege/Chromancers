// Vertex shader
// The output of a vertex shader is the position of the vertex
// after any kind of elaboration
// #version 410 core

layout (location = 0) in vec3 pos;

out gl_PerVertex { vec4 gl_Position; };

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
   // note that we read the multiplication from right to left
   gl_Position = u_proj * u_view * u_model * vec4(pos, 1.0);
}