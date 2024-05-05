#version 430 core

// output variable for the fragment shader
layout (location = 0) out vec4 g_color;
layout (location = 1) out vec4 g_wFragPos;
layout (location = 2) out vec4 g_wNormal;

in VS_OUT 
{
	// world space data
	vec3 wFragPos;
	vec3 wNormal;
} fs_in;

uniform vec4 diffuse_color;

void main()
{
	g_color     = vec4(diffuse_color);
	g_wFragPos = vec4(fs_in.wFragPos, 1);
	g_wNormal  = vec4(fs_in.wNormal, 1);
}