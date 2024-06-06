#version 430 core

out vec4 color; // output shader variable

// Basic fragment shader, assigns a yellow color to the fragment 
void main()
{
    color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
