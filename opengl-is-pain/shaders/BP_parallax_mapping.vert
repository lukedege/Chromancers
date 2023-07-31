#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates
layout (location = 3) in vec3 tangent;   // tangent   axis in tangent space (orthogonal to normal)
layout (location = 4) in vec3 bitangent; // bitangent axis in tangent space (orthogonal to normal)

out VS_OUT 
{
	// tangent space data
	vec3 twPointLightPos[MAX_POINT_LIGHTS];
	vec3 twFragPos; // Local -> World -> Tangent position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twCameraPos; 
	vec3 tNormal;

	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix; //this is the normal matrix multiplied by the viewprojection by scene object
// we dont need this now as we get our normals from the normal map

//Lights
uniform uint nPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform vec3 wCameraPos;

mat3 invTBN; // Inverse TangentBitangentNormal space transformation matrix

void calcPointLI(uint plIndex)
{
	vs_out.twPointLightPos[plIndex] = invTBN * pointLights[plIndex].position; // convert light position from world to tangent coordinates
}

void main()
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
	invTBN = transpose(mat3(T, B, N)); 

	vs_out.twFragPos = invTBN * vec3(modelMatrix * vec4(position, 1));;
	vs_out.interp_UV = UV;
	vs_out.tNormal = N;
	vs_out.twCameraPos = invTBN * wCameraPos;
	
	for(int i = 0; i < nPointLights; i++)
		calcPointLI(i);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}
