/*

01_fullcolor.frag : basic Fragment shader, it applies an uniform color to all the fragments. Color is passed as uniform from the main application

N.B.)  "00_basic.vert" must be used as vertex shader

*/

// #version 410 core

// output shader variable
out vec4 colorFrag;

// color to assign to the fragments: it is passed from the application
uniform vec3 colorIn;

void main()
{
    colorFrag = vec4(colorIn,1.0);
}
