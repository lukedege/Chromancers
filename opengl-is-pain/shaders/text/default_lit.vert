#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates
layout (location = 3) in vec3 tangent;   // tangent   axis in tangent space (orthogonal to normal)
layout (location = 4) in vec3 bitangent; // bitangent axis in tangent space (orthogonal to normal)

out VS_OUT 
{
	// tangent space data
	vec3 twPointLightDir[MAX_POINT_LIGHTS];
	vec3 twDirLightDir  [MAX_DIR_LIGHTS];
	vec3 twFragPos; // Local -> World -> Tangent position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twCameraPos; 
	vec3 twNormal;

	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

uniform mat4 modelMatrix      = mat4(1);
uniform mat4 viewMatrix       = mat4(1);
uniform mat4 projectionMatrix = mat4(1);
uniform mat3 normalMatrix     = mat3(1); //this is the normal matrix multiplied by the viewprojection by scene object

//Lights
uniform uint nPointLights;
uniform uint nDirLights;
uniform uint nSpotLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];
uniform SpotLight        spotLights       [MAX_SPOT_LIGHTS];

uniform vec3 wCameraPos = vec3(0);

vec3 wFragPos; // World fragment position
mat3 invTBN; // Inverse TangentBitangentNormal space transformation matrix

void calculatePointLightTangentDir(uint plIndex)
{
	vec3 curr_wPointLightDir = pointLights[plIndex].position - wFragPos; // Calculate light direction in world coordinates
	vs_out.twPointLightDir[plIndex] = invTBN * curr_wPointLightDir; // Convert light direction to tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light

	// Alternative method (convert all to tangent then calculate direction)
	//vec3 curr_twPointLightPos = invTBN * pointLights[plIndex].position; // convert light position from world to tangent coordinates
	//vs_out.twPointLightDir[plIndex] = curr_twPointLightPos - vs_out.twFragPos; // calculate light direction in tangent coordinates
}

void calculateDirLightTangentDir(uint plIndex)
{
	vs_out.twDirLightDir[plIndex] = invTBN * -directionalLights[plIndex].direction; // calculate light direction in tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light
}

void calculateSpotLightTangentDir(uint plIndex)
{
	
}

mat3 calculateInverseTBN(mat3 worldNormalMatrix, vec3 normal_vec, vec3 tangent_vec, vec3 bitangent_vec)
{
	vec3 N = normalize(worldNormalMatrix * normal_vec);
	vec3 T = normalize(worldNormalMatrix * tangent_vec);
	vec3 B = normalize(worldNormalMatrix * bitangent_vec);
    T = normalize(T - dot(T, N) * N);
    vec3 reortho_B = cross(N, T);
	if(dot(B, reortho_B) < 0) B = -reortho_B; // if they are opposite, adjust the ortho to match B facing and use the reorthogonalized value

	return transpose(mat3(T, B, N)); 
}

void main()
{
	// we calculate the inverse transform matrix to transform coords world space -> tangent space
	// we prefer calcs in the vertex shader since it is called less, thus less expensive computationally over time
	mat3 worldNormalMatrix = transpose(inverse(mat3(modelMatrix))); // this matrix updates normals to follow world/model matrix transformations
	invTBN = calculateInverseTBN(worldNormalMatrix, normal, tangent, bitangent); 

	wFragPos = vec3(modelMatrix * vec4(position, 1));

	vs_out.twFragPos = invTBN * wFragPos;
	vs_out.interp_UV = UV;
	vec3 wN = worldNormalMatrix * normal;
	vs_out.twNormal = normalize(invTBN * wN);
	vs_out.twCameraPos = invTBN * wCameraPos;
	
	for(uint i = 0; i < nPointLights; i++)
		calculatePointLightTangentDir(i);
	
	for(uint i = 0; i < nDirLights; i++)
		calculateDirLightTangentDir(i);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}
