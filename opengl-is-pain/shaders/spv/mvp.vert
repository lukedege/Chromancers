#version 460 core

layout (location = 0) in vec3 position;

out gl_PerVertex { vec4 gl_Position; };

layout (std140, binding = 0) uniform CameraMatrices
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

layout (std140, binding = 1) uniform ObjectMatrices
{
	mat4 modelMatrix;
	mat4 normalMatrix; // todo mat3
};

void main()
{
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position.x, position.y, position.z, 1.0f);
}