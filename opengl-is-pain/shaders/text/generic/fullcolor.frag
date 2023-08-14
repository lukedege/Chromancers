#version 410 core

// output shader variable
out vec4 colorFrag;

// color to assign to the fragments: it is passed from the application
uniform vec3 colorIn;

void main()
{
    colorFrag = vec4(colorIn,1.0);
}
