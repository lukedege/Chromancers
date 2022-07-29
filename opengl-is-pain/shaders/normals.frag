// #version 410 core

// output variable for the fragment shader. Usually, it is the final color of the fragment
out vec4 color;

uniform vec3  u_color_in;
uniform float u_weight;
uniform float u_time;

in vec3 interp_N;

void main()
{
  color = vec4(interp_N, 1.f);
}