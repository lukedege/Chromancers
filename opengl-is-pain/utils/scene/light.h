#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../shader.h"
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

		virtual void setLightAttributes(GLuint ubo_lights, size_t offset)
		{
			glBindBuffer(GL_UNIFORM_BUFFER, ubo_lights);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::vec4), glm::value_ptr(color));
			glBufferSubData(GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4), sizeof(float), &intensity);
			

			//opengl::update_buffer_object(ubo_lights, GL_UNIFORM_BUFFER, offset                    , sizeof(glm::vec4), 1, (void*)glm::value_ptr(color));
			//opengl::update_buffer_object(ubo_lights, GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4), sizeof(float)    , 1, (void*)&intensity);
		}

		virtual void setup(const Shader& shader, size_t index) = 0;
		virtual void setup(GLuint ubo_lights   , size_t index) = 0;
		static size_t shader_sizeof() noexcept { return 0; }
	};

	class PointLight : public Light
	{
		struct PointLight_shader
		{
			glm::vec4 color;
			float intensity;

			glm::vec3 position;
		};
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

		void setup(GLuint ubo_lights, size_t index) override
		{
			size_t offset = shader_sizeof() * index;
			setLightAttributes(ubo_lights, offset);
			opengl::update_buffer_object(ubo_lights, GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4) + sizeof(float), sizeof(glm::vec3), 1, (void*)glm::value_ptr(position));
		}

		static size_t shader_sizeof() noexcept
		{
			return sizeof(PointLight_shader);
		}
	};

	class DirectionalLight : public Light
	{
		struct DirectionalLight_shader
		{
			glm::vec4 color;
			float intensity;

			glm::vec3 direction;
		};
	public:
		glm::vec3 direction;

		DirectionalLight(const glm::vec3& direction, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }) :
			Light{ color, intensity }, direction{ glm::normalize(direction) } {}

		void setup(const Shader& shader, size_t index) override
		{
			std::string prefix = "directionalLights[" + std::to_string(index) + "].";
			setLightAttributes(shader, prefix);
			shader.setVec3(prefix + "direction", direction);
		}

		void setup(GLuint ubo_lights, size_t index) override
		{
			size_t offset = shader_sizeof() * index;
			glBindBuffer(GL_UNIFORM_BUFFER, ubo_lights);
			glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::vec4), glm::value_ptr(color));
			glBufferSubData(GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4), sizeof(float), &intensity);
			glBufferSubData(GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4) + sizeof(float), sizeof(glm::vec3), glm::value_ptr(direction));
			//opengl::update_buffer_object(ubo_lights, GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4) + sizeof(float), sizeof(glm::vec3), 1, (void*)glm::value_ptr(direction));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		static size_t shader_sizeof() noexcept
		{
			return sizeof(DirectionalLight_shader);
		}
	};

	class SpotLight : public Light
	{
		struct SpotLight_shader
		{
			glm::vec4 color;
			float intensity;

			glm::vec3 position;
			glm::vec3 direction;
			float cutoffAngle;
		};
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

		void setup(GLuint ubo_lights, size_t index) override
		{
			size_t offset = shader_sizeof() * index;
			setLightAttributes(ubo_lights, offset);
			// TODO
			//opengl::update_buffer_object(ubo_lights, GL_UNIFORM_BUFFER, offset + sizeof(glm::vec4) + sizeof(float), sizeof(glm::vec3), 1, (void*)glm::value_ptr(position));
		}

		static size_t shader_sizeof() noexcept
		{
			return sizeof(SpotLight_shader);
		}
	};
}