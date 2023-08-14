#version 430 core

layout (location = 0) in vec3 position; // vertex position in world coordinates
layout (location = 1) in vec3 normal; // vertex normal
layout (location = 2) in vec2 UV; // UV texture coordinates

out VS_OUT 
{
	// the output arrays for light incidence
	vec3 pointLightDir[MAX_POINT_LIGHTS];
	vec3 directionalLightDir[MAX_POINT_LIGHTS];
	vec3 spotLightDir[MAX_POINT_LIGHTS];

	vec3 vNormal;
	vec3 vViewPosition;

	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

//Lights
uniform uint nPointLights;
uniform uint nDirLights;
uniform uint nSpotLights;

uniform PointLight        pointLights[MAX_POINT_LIGHTS];
uniform DirectionalLight  directionalLights[MAX_DIR_LIGHTS];
uniform SpotLight         spotLights[MAX_SPOT_LIGHTS];

// Model-view position
vec4 mvPosition;

void calcPointLightDir(uint plIndex)
{
	vec4 vLightPos = viewMatrix * vec4(pointLights[plIndex].position, 1); // convert light position from world to view coordinates
	vs_out.pointLightDir[plIndex] = vLightPos.xyz - mvPosition.xyz; // vector from light to vertex position in view coords
}

//void calcDirLI(uint dlIndex)
//{
//	// TODO
//}
//
//void calcSpotLI(uint slIndex)
//{
//	// TODO
//}

void main()
{
	mvPosition = viewMatrix * modelMatrix * vec4(position, 1);
	vec3 N = normalize(normalMatrix * normal);
	
	// I assign the values to a variable with "out" qualifier so to use the per-fragment interpolated values in the Fragment shader
	vs_out.interp_UV = UV;
	vs_out.vViewPosition = -mvPosition.xyz; // it would be camera_pos - vertex_pos in view coords, but camera in view coords is the origin thus is zero
	vs_out.vNormal = N;

	for(int i = 0; i < nPointLights; i++)
		calcPointLightDir(i);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * mvPosition;
}