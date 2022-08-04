#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/shader.h"
namespace utils::graphics::opengl
{

	struct LightAttributes
	{
		// Light values
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;

		// Multipliers 
		float kA;
		float kD;
		float kS;
	};

	class Light
	{
	public:
		LightAttributes attrs;

		Light(LightAttributes& attrs) : attrs(attrs) {}

		virtual void setLightAttrs(const Shader& shader, const std::string& prefix)
		{
			shader.setVec3(prefix + "lightAttrs.diffuse", attrs.diffuse);
			shader.setVec3(prefix + "lightAttrs.specular", attrs.specular);
			shader.setVec3(prefix + "lightAttrs.ambient", attrs.ambient);

			shader.setFloat(prefix + "lightAttrs.kA", attrs.kA);
			shader.setFloat(prefix + "lightAttrs.kD", attrs.kD);
			shader.setFloat(prefix + "lightAttrs.kS", attrs.kS);
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

		PointLight(glm::vec3 position, LightAttributes& attrs) :
			Light{ attrs }, position{ position } {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "pointLights[" + std::to_string(index) + "].";
			setLightAttrs(shader, prefix);
			shader.setVec3(prefix + "position", position);
		}
	};

	class DirectionalLight : Light
	{
	public:
		glm::vec3 direction;

		DirectionalLight(const glm::vec3& direction, LightAttributes& attrs) :
			Light(attrs), direction(direction) {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "directionalLights[" + std::to_string(index) + "].";
			setLightAttrs(shader, prefix);
			shader.setVec3(prefix + "direction", direction);
		}
	};

	class SpotLight : Light
	{
	public:
		glm::vec3 position;
		glm::vec3 direction;
		float cutoffAngle;

		SpotLight(const glm::vec3& position, const glm::vec3& direction, float cutoffAngle, LightAttributes& attrs) :
			Light(attrs), position(position), direction(direction), cutoffAngle(cutoffAngle) {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "spotLights[" + std::to_string(index) + "].";
			setLightAttrs(shader, prefix);
			shader.setVec3(prefix + "position", position);
			shader.setVec3(prefix + "direction", direction);
			shader.setFloat(prefix + "cutoffAngle", cutoffAngle);
		}
	};
}