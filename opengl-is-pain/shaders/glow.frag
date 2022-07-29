// Fragment shader
// The output of a fragment shader is the color of the generated fragments
// #version 410 core

out vec4 color;

uniform float u_time;

void main()
{
   float c = abs(sin(u_time))+0.1f;
   color = vec4(c, 0.f, c, 1.0f);
}