#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates
layout (location = 3) in vec3 tangent;   // tangent   axis in tangent space (orthogonal to normal)
layout (location = 4) in vec3 bitangent; // bitangent axis in tangent space (orthogonal to normal)

out VS_OUT 
{
	// tangent space data
	vec3 tPointLightDir[MAX_POINT_LIGHTS];
	vec3 tViewPosition;

	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

//Lights
uniform uint nPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform vec3 cameraPos;

mat3 invTBN; // Inverse TangentBitangentNormal space transformation matrix
vec3 mtPosition; // Model-tangent position (aka position in tangent space)
vec4 mvPosition; // Model-view position (aka position in view space)

void calcPointLI(uint plIndex)
{
	vec3 tLightPos = invTBN * pointLights[plIndex].position; // convert light position from world to tangent coordinates

	vs_out.tPointLightDir[plIndex] = tLightPos - mtPosition.xyz; // vector from light to vertex position in tangent coords
}

void main()
{
	vec3 N = normalize(normalMatrix * normal);
	vec3 T = normalize(normalMatrix * tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	// we calculate the inverse transform matrix to transform coords world space -> tangent space
	// we prefer calcs in the vertex shader since it is called less, thus less expensive computationally over time
	invTBN = transpose(mat3(T, B, N)); 

	mvPosition = viewMatrix * modelMatrix * vec4(position, 1);
	mtPosition = invTBN * vec3(modelMatrix * vec4(position, 1));

	vs_out.interp_UV = UV;

	vec3 tCameraPos = invTBN * cameraPos;
	vs_out.tViewPosition = tCameraPos - mtPosition; // it would be camera_pos - vertex_pos in view coords, but camera in view coords is the origin thus is zero

	for(int i = 0; i < nPointLights; i++)
		calcPointLI(i);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * mvPosition;
}
