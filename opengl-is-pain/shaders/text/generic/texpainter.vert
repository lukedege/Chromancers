#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates
layout (location = 3) in vec3 tangent;   // tangent   axis in tangent space (orthogonal to normal)
layout (location = 4) in vec3 bitangent; // bitangent axis in tangent space (orthogonal to normal)

// Paint transform matrix
uniform mat4 paintSpaceMatrix;

// Matrix of the model.
uniform mat4 modelMatrix;

// The direction of the paintball in world coordinates.
uniform vec3 paintBallDirection;

out VS_OUT 
{
    // the output variable for UV coordinates
	vec2 interp_UV;

    // vertex normal
    vec3 normal;

    // paint dir
    vec3 wPaintDir;

    // Fragment position in paint space
    vec4 pwFragPos;
} vs_out;

void main()
{
    
    vs_out.interp_UV = UV;
    vs_out.normal = normalize(normal);
    vs_out.wPaintDir = normalize(paintBallDirection);
    vs_out.pwFragPos = paintSpaceMatrix * modelMatrix * vec4(position, 1.0f);

    gl_Position = vs_out.pwFragPos;
}