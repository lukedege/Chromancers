#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../io.h"
#include "../shader.h"
#include "../texture.h"
#include "../framebuffer.h"
#include "scene.h"

namespace engine::scene
{
	class Light
	{
	protected:
		using Shader = engine::resources::Shader;
		using Texture = engine::resources::Texture;
	public:
		struct ShadowMapSettings
		{
			Shader* shader = nullptr; 
			unsigned int resolution = 1024;
			float frustum_size = 50.f;
			float frustum_near = 1.f; 
			float frustum_far = 50.f;
			float distance_bias = 20.f;
		} shadowmap_settings;

		utils::graphics::opengl::Framebuffer shadows_framebuffer;

		glm::vec4 color    ;
		float     intensity;
	
		Light(const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = 1.0f, ShadowMapSettings shadowmap_settings = {}) : 
			color{ color }, intensity{ intensity }, 
			shadowmap_settings{ shadowmap_settings },
			shadows_framebuffer{ shadowmap_settings.resolution, shadowmap_settings.resolution, 
			Texture::FormatInfo{GL_RGB, GL_RGB, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} }
		{}

		virtual void compute_shadowmap(const engine::scene::Scene& scene, Shader* vis_shader) {};
		virtual void setup(const Shader& shader, size_t index) = 0;

		const Texture& get_shadowmap()
		{
			return shadows_framebuffer.get_depth_attachment();
		}

	protected:
		void setBaseLightAttributes(const Shader& shader, const std::string& prefix)
		{
			shader.setVec4 (prefix + "color", color);
			shader.setFloat(prefix + "intensity", intensity);
		}
	};

	class PointLight : public Light
	{
	public:
		glm::vec3 position;

		// Attenuation values 
		float attenuation_constant    = 1.f;
		float attenuation_linear      = 0.5f;
		float attenuation_quadratic   = 0.01f;

		PointLight(const glm::vec3& position, 
			const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }, ShadowMapSettings shadowmap_settings = {}) :
			Light{ color, intensity, shadowmap_settings }, 
			position{ position } 
		{}

		void setup(const Shader& shader, size_t index)
		{
			std::string prefix = "pointLights[" + std::to_string(index) + "].";
			setBaseLightAttributes(shader, prefix);
			shader.setVec3 (prefix + "position", position);
			shader.setFloat(prefix + "attenuation_constant" , attenuation_constant);
			shader.setFloat(prefix + "attenuation_linear"   , attenuation_linear);
			shader.setFloat(prefix + "attenuation_quadratic", attenuation_quadratic);
		}
	};

	class DirectionalLight : public Light
	{
	public:
		glm::vec3 direction;

		DirectionalLight(const glm::vec3& direction, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float intensity = 1.0f, ShadowMapSettings shadowmap_settings = {}) :
			Light{ color, intensity, shadowmap_settings }, 
			direction{ glm::normalize(direction) }
		{}

		void setup(const Shader& shader, size_t index)
		{
			std::string prefix = "directionalLights[" + std::to_string(index) + "].";
			setBaseLightAttributes(shader, prefix);
			shader.setVec3(prefix + "direction", direction);
		}

		void compute_shadowmap(const engine::scene::Scene& scene, Shader* lit_shader /*TODO temp!!*/) override
		{
			if (!shadowmap_settings.shader)
			{
				utils::io::warn("DIR LIGHT - Shader for shadow map computation not set!");
				return;
			}
		
			float frustum_size = shadowmap_settings.frustum_size;
			glm::mat4 lightProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, shadowmap_settings.frustum_near, shadowmap_settings.frustum_far);
			glm::mat4 lightView = glm::lookAt(-direction * shadowmap_settings.distance_bias, glm::vec3(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;
			
			// TODO temp: each light has its own lightspacematrix, thus should go in the array of light attributes in the shader
			lit_shader->bind();
			lit_shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
			lit_shader->unbind();

			// Shadow pass
			shadows_framebuffer.bind();
			glClear(GL_DEPTH_BUFFER_BIT);
		
			glCullFace(GL_FRONT); // Culling front face to reduce peter panning effect
		
			shadowmap_settings.shader->bind();
			shadowmap_settings.shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
			scene.custom_draw(*shadowmap_settings.shader);
			shadows_framebuffer.unbind();
		
			glCullFace(GL_BACK); // Restoring back face culling for drawing
		}

	};

	class SpotLight : public Light
	{
	public:
		glm::vec3 position;
		glm::vec3 direction;
		float cutoffAngle;

		SpotLight(const glm::vec3& position, const glm::vec3& direction, float cutoffAngle, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = 1.0f ) :
			Light{ color, intensity, shadowmap_settings }, 
			position{ position }, direction{ direction }, cutoffAngle{ cutoffAngle } {}

		void setup(const Shader& shader, size_t index)
		{
			std::string prefix = "spotLights[" + std::to_string(index) + "].";
			setBaseLightAttributes(shader, prefix);
			shader.setVec3 (prefix + "position", position);
			shader.setVec3 (prefix + "direction", direction);
			shader.setFloat(prefix + "cutoffAngle", cutoffAngle);
		}
	};
}