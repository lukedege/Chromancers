#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/shader.h"
namespace utils::graphics::opengl
{

	class Light
	{
	public:
		glm::vec4 color    ;
		float     intensity;

		Light(const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }) : color{ color }, intensity{ intensity } {}

		virtual void setLightAttributes(const Shader& shader, const std::string& prefix)
		{
			shader.setVec4 (prefix + "color", color);
			shader.setFloat(prefix + "intensity", intensity);
		}
		virtual void setup(const Shader& shader, size_t index) = 0;
	};

	class PointLight : Light
	{
	public:
		glm::vec3 position;

		/* Decay values TODO LATER
		float constant    = 1.f;
		float linear      = 1.f;
		float quadratic   = 1.f;*/

		PointLight(const glm::vec3& position, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }) :
			Light{ color, intensity }, position{ position } {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "pointLights[" + std::to_string(index) + "].";
			setLightAttributes(shader, prefix);
			shader.setVec3(prefix + "position", position);
		}
	};

	class DirectionalLight : Light
	{
	public:
		glm::vec3 direction;

		DirectionalLight(const glm::vec3& direction, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }) :
			Light{ color, intensity }, direction{ direction } {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "directionalLights[" + std::to_string(index) + "].";
			setLightAttributes(shader, prefix);
			shader.setVec3(prefix + "direction", direction);
		}
	};

	class SpotLight : Light
	{
	public:
		glm::vec3 position;
		glm::vec3 direction;
		float cutoffAngle;

		SpotLight(const glm::vec3& position, const glm::vec3& direction, float cutoffAngle, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }) :
			Light{ color, intensity }, position{ position }, direction{ direction }, cutoffAngle{ cutoffAngle } {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "spotLights[" + std::to_string(index) + "].";
			setLightAttributes(shader, prefix);
			shader.setVec3 (prefix + "position", position);
			shader.setVec3 (prefix + "direction", direction);
			shader.setFloat(prefix + "cutoffAngle", cutoffAngle);
		}
	};
}