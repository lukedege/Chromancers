// #version 410 core
// TODO watch out for data alignment and similar errors coming from it

struct PointLight
{
	vec4  color;
	float intensity;
	
	vec3  position;
};

struct DirectionalLight
{
	vec4  color;
	float intensity;

	vec3 direction;
};

struct SpotLight
{
	vec4  color;
	float intensity;

	vec3 position;
	vec3 direction;
	float cutoffAngle;
};