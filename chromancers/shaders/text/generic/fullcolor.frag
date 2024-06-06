#version 430 core

out vec4 color;

uniform vec3 colorIn;

// Simple fragment shader, retrieves from a uniform the color to assign to the fragment 
void main()
{
    color = vec4(colorIn, 1.0);
}
