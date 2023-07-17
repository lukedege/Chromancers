#version 460 core

#include "../types.glsl"
#include "../constants.glsl"

layout (location = 0) in vec3 position; // vertex position in world coordinates
layout (location = 1) in vec3 normal; // vertex normal
layout (location = 2) in vec2 UV; // UV texture coordinates

//TODO move matrices into uniform buffer instead of single uniforms
layout (std140, binding = 0) uniform CameraMatrices
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

layout (std140, binding = 1) uniform ObjectMatrices
{
	mat4 modelMatrix;
	mat3 normalMatrix;
	float fill;
};

layout (std140, binding = 2) uniform LightsAmount
{
	uint nPointLights;
	uint nDirLights;
	uint nSpotLights;
};

layout (std140, binding = 3) uniform LightsData
{
	PointLight        pointLights[MAX_POINT_LIGHTS];
 	//DirectionalLight  directionalLights[MAX_DIR_LIGHTS];
 	//SpotLight         spotLights[MAX_SPOT_LIGHTS];
};

layout(location = 20) out LightIncidence   pointLI[MAX_POINT_LIGHTS];
layout(location = 40) out LightIncidence   dirLI  [MAX_DIR_LIGHTS];
layout(location = 60) out LightIncidence   spotLI [MAX_SPOT_LIGHTS];

// the output variable for UV coordinates
layout(location = 10) out vec2 interp_UV;

// Model-view position
vec4 mvPosition;

void calcPointLI(uint plIndex)
{
	pointLI[plIndex].vViewPosition = -mvPosition.xyz; // it would be camera_pos - vertex_pos in view coords, but camera in view coords is the origin thus is zero
	
	vec4 lightPos = viewMatrix * vec4(pointLights[plIndex].position, 1); // convert light position from world to view coordinates
	pointLI[plIndex].lightDir = lightPos.xyz - mvPosition.xyz; // vector from light to vertex position in view coords
	
	pointLI[plIndex].vNormal = normalize(normalMatrix * normal);
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
	// I assign the values to a variable with "out" qualifier so to use the per-fragment interpolated values in the Fragment shader
	interp_UV = UV;

	for(int i = 0; i < nPointLights; i++)
		calcPointLI(i);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * mvPosition;
}
