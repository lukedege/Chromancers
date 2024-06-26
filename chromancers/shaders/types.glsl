// #version 410 core

// Utility shader containing structure definitions for shader usage

struct PointLight
{
	vec4  color;
	float intensity;
	mat4 lightspace_matrix; // light space matrix is already lightProjMatrix * lightViewMatrix

	float attenuation_constant ;
	float attenuation_linear   ;
	float attenuation_quadratic;
	
	vec3  position;
};

struct DirectionalLight
{
	vec4  color;
	float intensity;
	mat4 lightspace_matrix;

	vec3 direction;
};

struct SpotLight
{
	vec4  color;
	float intensity;
	mat4 lightspace_matrix;

	vec3 position;
	vec3 direction;
	float cutoffAngle;
};