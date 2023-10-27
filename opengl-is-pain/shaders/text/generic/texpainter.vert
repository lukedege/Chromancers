#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates

// Paint transform matrix
uniform mat4 paintSpaceMatrix;

// Matrix of the model.
uniform mat4 modelMatrix;

out VS_OUT 
{
    // the output variable for UV coordinates
	vec2 interp_UV;

    // vertex normal
    vec3 wNormal;

    // Fragment position in paint space
    vec4 pwFragPos;
} vs_out;

void main()
{
    mat3 worldNormalMatrix = transpose(inverse(mat3(modelMatrix))); // this matrix updates normals to follow world/model matrix transformations

    vs_out.interp_UV = UV;

    vs_out.wNormal = normalize(worldNormalMatrix * normal);
    vs_out.pwFragPos = paintSpaceMatrix * modelMatrix * vec4(position, 1.0f);

    gl_Position = vs_out.pwFragPos;
}