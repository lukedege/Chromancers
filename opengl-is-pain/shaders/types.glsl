// #version 410 core
// TODO watch out for data alignment and similar errors coming from it

struct LightIncidence
{
	vec3 lightDir;
	vec3 vNormal;
	vec3 vViewPosition;
};

struct PointLight
{
	vec3  position;

	float intensity;
	vec4  color;
};

struct DirectionalLight
{
	vec3 direction;
	
	float intensity;
	vec4  color;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float cutoffAngle;
	
	float intensity;
	vec4  color;
};