#version 460 core

#include "../types.glsl"
#include "../constants.glsl"

// output variable for the fragment shader
layout(location = 0) out vec4 colorFrag;

// the output variable for UV coordinates
layout(location = 10) in vec2 interp_UV;

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

layout(location = 20) in LightIncidence   pointLI[MAX_POINT_LIGHTS];
layout(location = 40) in LightIncidence   dirLI  [MAX_DIR_LIGHTS];
layout(location = 60) in LightIncidence   spotLI [MAX_SPOT_LIGHTS];


LightIncidence  currLI;

layout (std140, binding = 4) uniform ShadingAttributes
{
	// Material-light attributes (for now the whole scene is a single material)
	uniform vec3 ambient ;
	uniform vec3 diffuse ;
	uniform vec3 specular;

	// Ambient, diffuse, specular coefficients
	uniform float kA;
	uniform float kD;
	uniform float kS;

	// shininess coefficients (passed from the application)
	uniform float shininess;

	// uniforms for GGX model
	uniform float alpha; // rugosity - 0 : smooth, 1: rough
	uniform float F0; // fresnel reflectance at normal incidence
};

vec3 BlinnPhong()
{
	// ambient component can be calculated at the beginning
	vec3 color = kA * ambient;
	
	// normalization of the per-fragment normal
	vec3 N = normalize(currLI.vNormal);
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(currLI.lightDir);
	
	// Lambert coefficient
	float lambertian = max(dot(L,N), 0.0);
	
	// if the lambert coefficient is positive, then I can calculate the specular component
	if(lambertian > 0.0)
	{
		// the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
		vec3 V = normalize( currLI.vViewPosition );
		
		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float specular = pow(specAngle, shininess);
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		color += vec3( kD * lambertian * diffuse + kS * specular   * specular);
	}
	return color;
}

vec4 calculatePointLights()
{
	vec4 color = vec4(0);
	for(int i = 0; i < nPointLights; i++)
	{
		currLI = pointLI[i];
		color += vec4(BlinnPhong(), 1.0f) * pointLights[i].color * pointLights[i].intensity;
	}
	color.a = normalize(color.a);
	return color;
}

vec4 calculateDirectionalLights()
{
	vec4 color;
	// TODO
	return color;
}

vec4 calculateSpotLights()
{
	vec4 color;
	// TODO
	return color;
}

void main()
{
	vec4 color = vec4(0);
	
	color += calculatePointLights();
	//color += calculateDirectionalLights();
	//color += calculateSpotLights();
	
	colorFrag = color;
}