#version 430 core

layout (location = 0) in vec3 position;  // vertex position in world coordinates
layout (location = 1) in vec3 normal;    // vertex normal

out VS_OUT 
{
	// world space data
	vec3 wFragPos;
	vec3 wNormal;
} vs_out;

layout (std430, binding = 0) buffer InstanceGroupTransforms
{
	mat4 instance_transforms[];
};

uniform mat4 modelMatrix      = mat4(1);
uniform mat4 viewMatrix       = mat4(1);
uniform mat4 projectionMatrix = mat4(1);

void main()
{
	// we use instanceID to find the instance transformation matrix in the array containing all the transforms for the instance group
	// fragment should be exactly like this since we only touch the position of the vertex based on the instance transform
	mat4 worldMatrix = instance_transforms[gl_InstanceID] * modelMatrix;
	
	mat3 worldNormalMatrix = transpose(inverse(mat3(worldMatrix))); // this matrix updates normals to follow world/model matrix transformations

	// calculate world space output data, we dont need tangent data as we aren't using normal maps for paintballs
	vs_out.wFragPos = vec3(worldMatrix * vec4(position, 1)); 
	vs_out.wNormal = normalize(worldNormalMatrix * normal);

	// transformations are applied to each vertex
	gl_Position = projectionMatrix * viewMatrix * worldMatrix * vec4(position, 1.0);
}
