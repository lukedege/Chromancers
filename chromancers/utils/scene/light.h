#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../io.h"
#include "../shader.h"
#include "../texture.h"
#include "../framebuffer.h"
#include "scene.h"

#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3
#define MAX_DIR_LIGHTS   3
#define MAX_LIGHTS MAX_POINT_LIGHTS+MAX_SPOT_LIGHTS+MAX_DIR_LIGHTS

namespace engine::scene
{
	class Light
	{
	protected:
		using Shader = engine::resources::Shader;
		using Texture = engine::resources::Texture;
	public:

		glm::vec4 color    ;
		float     intensity;
	
		Light(const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = 1.0f) : 
			color{ color }, intensity{ intensity }
		{}

		virtual void compute_shadowmap(const engine::scene::Scene& scene) {};
		virtual void setup(const Shader& shader, size_t index) = 0;

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
		struct ShadowMapSettings
		{
			Shader* shader = nullptr; 
			unsigned int resolution = 1024;
			float frustum_near = 1.f; 
			float frustum_far = 25.f;
			float distance_bias = 20.f;
		} shadowmap_settings;

		glm::vec3 position;

		// Attenuation values 
		float attenuation_constant    = 0.1f;
		float attenuation_linear      = 0.1f;
		float attenuation_quadratic   = 0.02f;

		// Shadowmap stuff
		utils::graphics::opengl::BasicFramebuffer depthmap_framebuffer;
		unsigned int depthCubemap;

		PointLight(const glm::vec3& position, 
			const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = { 1.0f }, ShadowMapSettings shadowmap_settings = {}) :
			Light{ color, intensity }, 
			shadowmap_settings{ shadowmap_settings },
			depthmap_framebuffer{ shadowmap_settings.resolution, shadowmap_settings.resolution },
			position{ position } 
		{
			// Create cubemap
			glGenTextures(1, &depthCubemap);

			const unsigned int SHADOW_WIDTH = shadowmap_settings.resolution, SHADOW_HEIGHT = shadowmap_settings.resolution;
			glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

			// Create a 2D texture for each face of the cubemap
			for (unsigned int i = 0; i < 6; ++i)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 
								 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  

			// Attach the whole cubemap as depth attachment to the framebuffer as we'll do a geom shader trick to render all faces at once
			depthmap_framebuffer.bind();
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
			glDrawBuffer(GL_NONE); // NO COLOR ATTACHMENT NEEDED
			glReadBuffer(GL_NONE);
			depthmap_framebuffer.unbind();
		}

		void setup(const Shader& shader, size_t index)
		{
			std::string prefix = "pointLights[" + std::to_string(index) + "].";
			setBaseLightAttributes(shader, prefix);
			shader.setVec3 (prefix + "position", position);
			shader.setFloat(prefix + "attenuation_constant" , attenuation_constant);
			shader.setFloat(prefix + "attenuation_linear"   , attenuation_linear);
			shader.setFloat(prefix + "attenuation_quadratic", attenuation_quadratic);
		}

		void compute_shadowmap(const engine::scene::Scene& scene) override
		{
			if (!shadowmap_settings.shader)
			{
				utils::io::warn("POINT LIGHT - Shader for shadow map computation not set!");
				return;
			}

			std::vector<glm::mat4> lightspace_matrices = compute_lightspace_matrices();
			const unsigned int SHADOW_WIDTH = shadowmap_settings.resolution, SHADOW_HEIGHT = shadowmap_settings.resolution;

			// Shadow pass
			depthmap_framebuffer.bind();
			glClear(GL_DEPTH_BUFFER_BIT);
		
			glCullFace(GL_FRONT); // Culling front face to reduce peter panning effect
		
			shadowmap_settings.shader->bind();
			// geom shader uniform setting
			for (int index = 0; index < lightspace_matrices.size(); index++)
			{
				shadowmap_settings.shader->setMat4("shadowMatrices["+std::to_string(index)+"]", lightspace_matrices[index]);
			}
			//frag shader uniform setting
			shadowmap_settings.shader->setVec3("lightPos", position);
			shadowmap_settings.shader->setFloat("far_plane", shadowmap_settings.frustum_far);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
			scene.custom_draw(*shadowmap_settings.shader);

			depthmap_framebuffer.unbind();
		
			glCullFace(GL_BACK); // Restoring back face culling for drawing
		}
	private:
		std::vector<glm::mat4> compute_lightspace_matrices()
		{
			const unsigned int SHADOW_WIDTH = shadowmap_settings.resolution, SHADOW_HEIGHT = shadowmap_settings.resolution;

			// We calculate it by perspective since the point light is part of the scene 
			// and with relative distance to its objects, unlike directional lights 
			float aspect = (float)SHADOW_WIDTH/(float)SHADOW_HEIGHT;
			float near = 1.0f;
			float far = 25.0f;
			// With a 90° fov we can ensure the viewing field fills the cube face and doesnt underflow or overflow
			glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far); 

			// Create a lightspace matrix for each face
			std::vector<glm::mat4> shadowTransforms;
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

			return shadowTransforms;
		}
	};

	class DirectionalLight : public Light
	{
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

		glm::vec3 direction;

		utils::graphics::opengl::Framebuffer depthmap_framebuffer;
		glm::mat4 lightspace_matrix{1};

		DirectionalLight(const glm::vec3& direction, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float intensity = 1.0f, ShadowMapSettings shadowmap_settings = {}) :
			Light{ color, intensity }, 
			direction{ glm::normalize(direction) },
			shadowmap_settings{ shadowmap_settings },
			depthmap_framebuffer{ shadowmap_settings.resolution, shadowmap_settings.resolution,
				Texture::FormatInfo{GL_RGB, GL_RGB, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} }
		{}

		void setup(const Shader& shader, size_t index)
		{
			std::string prefix = "directionalLights[" + std::to_string(index) + "].";
			lightspace_matrix = compute_lightspace_matrix();
			setBaseLightAttributes(shader, prefix);
			shader.setVec3(prefix + "direction", direction);
			shader.setMat4(prefix + "lightspace_matrix", lightspace_matrix);
		}

		void compute_shadowmap(const engine::scene::Scene& scene) override
		{
			if (!shadowmap_settings.shader)
			{
				utils::io::warn("DIR LIGHT - Shader for shadow map computation not set!");
				return;
			}
			
			// Shadow pass
			depthmap_framebuffer.bind();
			glClear(GL_DEPTH_BUFFER_BIT);
		
			glCullFace(GL_FRONT); // Culling front face to reduce peter panning effect
		
			shadowmap_settings.shader->bind();
			shadowmap_settings.shader->setMat4("lightSpaceMatrix", lightspace_matrix);
			scene.custom_draw(*shadowmap_settings.shader);
			depthmap_framebuffer.unbind();
		
			glCullFace(GL_BACK); // Restoring back face culling for drawing
		}

		const Texture& get_shadowmap()
		{
			return depthmap_framebuffer.get_depth_attachment();
		}

	private:
		glm::mat4 compute_lightspace_matrix()
		{
			float frustum_size = shadowmap_settings.frustum_size;
			glm::mat4 lightProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, shadowmap_settings.frustum_near, shadowmap_settings.frustum_far);
			glm::mat4 lightView = glm::lookAt(-direction * shadowmap_settings.distance_bias, glm::vec3(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
			return lightProjection * lightView;
		}
	};

	class SpotLight : public Light
	{
	public:
		glm::vec3 position;
		glm::vec3 direction;
		float cutoffAngle;

		SpotLight(const glm::vec3& position, const glm::vec3& direction, float cutoffAngle, const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, const float intensity = 1.0f ) :
			Light{ color, intensity }, 
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