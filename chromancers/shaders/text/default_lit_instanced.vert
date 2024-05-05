#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal
layout (location = 2) in vec2 UV;        // UV texture coordinates
layout (location = 3) in vec3 tangent;   // tangent   axis in tangent space (orthogonal to normal)
layout (location = 4) in vec3 bitangent; // bitangent axis in tangent space (orthogonal to normal)

out VS_OUT 
{
	// world space data
	vec3 wFragPos;
	//vec3 wNormal;
	vec3 wPointLightDir[MAX_POINT_LIGHTS];
	vec3 wDirLightDir  [MAX_DIR_LIGHTS];

	// view space data
	vec3 vwFragPos;

	// tangent space data
	vec3 twFragPos; // Local -> World (w) -> Tangent (t) position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twPointLightDir[MAX_POINT_LIGHTS];
	vec3 twDirLightDir  [MAX_DIR_LIGHTS];
	vec3 twCameraPos; 
	vec3 twNormal;

	// light space data (l = transformed by lightProjMatrix * lightViewMatrix) (needed for shadow calcs)
	vec4 lwDirFragPos[MAX_DIR_LIGHTS]; // fragment position in light space for directional lights

	// the output variable for UV coordinates
	vec2 interp_UV;
} vs_out;

layout (std430, binding = 0) buffer InstanceGroupTransforms
{
	mat4 instance_transforms[];
};

uniform mat4 modelMatrix      = mat4(1);
uniform mat4 viewMatrix       = mat4(1);
uniform mat4 projectionMatrix = mat4(1);

//Lights
uniform uint nPointLights;
uniform uint nDirLights  ;
uniform uint nSpotLights ;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];
uniform SpotLight        spotLights       [MAX_SPOT_LIGHTS];

uniform vec3 wCameraPos = vec3(0);

mat3 invTBN; // Inverse TangentBitangentNormal space transformation matrix

void calculateDirLightTangentDir(uint light_idx)
{
	vs_out.wDirLightDir[light_idx] = -directionalLights[light_idx].direction;
	vs_out.twDirLightDir[light_idx] = invTBN * -directionalLights[light_idx].direction; // calculate light direction in tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light
}

void calculatePointLightTangentDir(uint light_idx)
{
	vec3 curr_wPointLightDir = pointLights[light_idx].position - vs_out.wFragPos; // Calculate light direction in world coordinates
	vs_out.wPointLightDir[light_idx] = curr_wPointLightDir;
	vs_out.twPointLightDir[light_idx] = invTBN * curr_wPointLightDir; // Convert light direction to tangent coordinates
	// the direction vector is intended as directed from the fragment towards the light

	// Alternative method (convert all to tangent then calculate direction)
	//vec3 curr_twPointLightPos = invTBN * pointLights[light_idx].position; // convert light position from world to tangent coordinates
	//vs_out.twPointLightDir[light_idx] = curr_twPointLightPos - vs_out.twFragPos; // calculate light direction in tangent coordinates
}

void calculateSpotLightTangentDir(uint light_idx)
{
	
}

void calculateDirLightSpaceFragPos(uint light_idx)
{
	vs_out.lwDirFragPos[light_idx] = directionalLights[light_idx].lightspace_matrix * vec4(vs_out.wFragPos, 1);
}

void calculatePointLightSpaceFragPos(uint light_idx)
{

}

void calculateSpotLightSpaceFragPos(uint light_idx)
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
	mat4 worldMatrix = instance_transforms[gl_InstanceID] * modelMatrix;
	// we calculate the inverse transform matrix to transform coords world space -> tangent space
	// we prefer calcs in the vertex shader since it is called less, thus less expensive computationally over time
	mat3 worldNormalMatrix = transpose(inverse(mat3(worldMatrix))); // this matrix updates normals to follow world/model matrix transformations
	invTBN = calculateInverseTBN(worldNormalMatrix, normal, tangent, bitangent); 

	// calculate world space output data
	// we use instanceID to find the instance transformation matrix in the array containing all the transforms for the instance group
	// fragment should be exactly like this since we only touch the position of the vertex based on the instance transform
	vs_out.wFragPos = vec3(worldMatrix * vec4(position, 1)); 

	// calculate view space output data
	vs_out.vwFragPos = vec3(viewMatrix * vec4(vs_out.wFragPos, 1));

	// calculate tangent space output data
	vs_out.twFragPos = invTBN * vs_out.wFragPos;
	//vs_out.wNormal = normalize(worldNormalMatrix * normal);
	vs_out.twNormal = normalize(invTBN * worldNormalMatrix * normal);
	vs_out.twCameraPos = invTBN * wCameraPos;

	vs_out.interp_UV = UV;
	
	// calculate light related data
	for(uint i = 0; i < nPointLights; i++)
	{
		calculatePointLightTangentDir(i);
	}

	for(uint i = 0; i < nDirLights; i++)
	{
		calculateDirLightTangentDir(i);
		calculateDirLightSpaceFragPos(i);
	}

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * viewMatrix * worldMatrix * vec4(position, 1.0);
}
