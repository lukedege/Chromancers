#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates

out VS_OUT 
{
	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

void main()
{
    vs_out.interp_UV = UV;

    gl_Position = vec4(position.x, position.y, 0, 1.0f);
}
