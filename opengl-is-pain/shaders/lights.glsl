// #version 410 core
// TODO watch out for data alignment and similar errors coming from it

#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3
#define MAX_DIR_LIGHTS   3
#define MAX_LIGHTS MAX_POINT_LIGHTS+MAX_SPOT_LIGHTS+MAX_DIR_LIGHTS

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

uniform uint nPointLights;
uniform uint nDirLights;
uniform uint nSpotLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];
uniform SpotLight        spotLights       [MAX_SPOT_LIGHTS];

mat3 calculate_TBN(mat4 modelMatrix)
{
	mat3 worldNormalMatrix = transpose(inverse(mat3(modelMatrix)));
	vec3 N = normalize(worldNormalMatrix * normal);
	vec3 T = normalize(worldNormalMatrix * tangent);
	vec3 B = normalize(worldNormalMatrix * bitangent);
    T = normalize(T - dot(T, N) * N);
    vec3 reortho_B = cross(N, T);
	if(dot(B, reortho_B) < 0) B = -reortho_B; // if they are opposite, adjust the ortho to match B facing and use the reorthogonalized value

	// we calculate the inverse transform matrix to transform coords world space -> tangent space
	// we prefer calcs in the vertex shader since it is called less, thus less expensive computationally over time
	return mat3(T, B, N);
}

mat3 calculate_invTBN()
{
	return transpose(calculate_TBN()); 
}

vec3 calculatePointLightTangentDir(vec3 wPointLight_position, vec3 wFrag_position, mat3 invTBN)
{

}

// Uses world coordinates
vec3 calculatePointLightTangentDir(vec3 wPointLight_position, vec3 wFrag_position, mat3 invTBN)
{
	vec3 curr_wPointLightDir = wPointLight_position - wFrag_position; // Calculate light direction in world coordinates
	
	return invTBN * curr_wPointLightDir; // Convert light direction to tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light
}

// Uses tangent-world coordinates
vec3 calculatePointLightTangentDir(vec3 twPointLight_position, vec3 twFrag_position)
{
	return twPointLight_position - twFrag_position; // Calculate light direction in tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light
}