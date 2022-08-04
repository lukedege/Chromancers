// #version 410 core

layout (location = 0) in vec3 position; // vertex position in world coordinates
layout (location = 1) in vec3 normal; // vertex normal
layout (location = 2) in vec2 UV; // UV texture coordinates

// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

// Model-view position
vec4 mvPosition;

//Lights
uniform uint nPointLights;
uniform uint nDirLights;
uniform uint nSpotLights;

uniform  PointLight        pointLights[MAX_POINT_LIGHTS];
out      LightIncidence        pointLI[MAX_POINT_LIGHTS];

uniform  DirectionalLight  directionalLights[MAX_DIR_LIGHTS];
out      LightIncidence                dirLI[MAX_DIR_LIGHTS];

uniform  SpotLight         spotLights[MAX_SPOT_LIGHTS];
out      LightIncidence        spotLI[MAX_SPOT_LIGHTS];

//uniform vec3 pointLightPosition;
//out vec3 lightDir; // we calculate per-vertex the direction of the light coming from the point light source
//out vec3 vNormal;  		// we pass the vertex normal vector
//out vec3 vViewPosition; // we pass the vector pointing to the camera, useful for specular component

// the output variable for UV coordinates
out vec2 interp_UV;

void calcPointLI(uint plIndex)
{
	pointLI[plIndex].vViewPosition = -mvPosition.xyz; // it would be camera pos - vertex pos in view coords, but camera in view coords is the origin thus is zero
	
	vec4 lightPos = viewMatrix * vec4(pointLights[plIndex].position, 1); // convert lightposition from world to view coordinates
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
