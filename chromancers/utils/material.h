#pragma once

#include <memory>

#include "shader.h"
#include "texture.h"

#define DIFFUSE_TEX_UNIT        0
#define NORMAL_TEX_UNIT         1
#define DISPLACEMENT_TEX_UNIT   2
#define DETAIL_DIFFUSE_TEX_UNIT 3
#define DETAIL_NORMAL_TEX_UNIT  4
#define SHADOW_TEX_UNIT         5

namespace engine::resources
{
	// Class for linking a shader to a set of various material properties and textures
	class Material
	{
		using Color = glm::vec4;
	public:
		Shader* shader              { nullptr };
		//Color albedo              { 1 }; TODO use in place of diffuse when ambient and specular are gone
		Texture* diffuse_map        { nullptr };
		Texture* normal_map         { nullptr };
		Texture* displacement_map   { nullptr };
		Texture* detail_diffuse_map { nullptr };
		Texture* detail_normal_map  { nullptr };

		// "Fake" lighting parameters
		float kA{ 0.1f }, kD{ 0.5f }, kS{ 0.4f };
		Color ambient_color { 0.1f, 0.1f, 0.1f, 1.f }, diffuse_color{ 1.0f, 1.0f, 1.0f, 1.f }, specular_color{ 1.0f, 1.0f, 1.0f, 1.f };
		float shininess   { 16.f };

		// SchlickGGX parameters
		float alpha { 0.2f }; // rugosity - 0 : smooth, 1: rough
		float F0    { 0.9f }; // fresnel reflectance at normal incidence

		// Texture parameters
		float uv_repeat { 1.f };

		// Displacement map parameters
		float parallax_heightscale { 0.05f };
		
		// Detail map related parameters
		float detail_diffuse_bias { 1.50f };
		float detail_normal_bias  { 0.25f };

		// Shadow map parameters
		bool receive_shadows{ true };

		Material() {}
		Material(Shader& shader) : shader { &shader } {}

		// Bind the shader and set all the relevant properties (including textures if they are available)
		void bind() const
		{
			if (!shader) { utils::io::error("MATERIAL - Shader not provided"); return; }
			
			shader->bind();

			shader->setVec4("ambient_color", ambient_color);
			shader->setVec4("diffuse_color", diffuse_color);
			shader->setVec4("specular_color", specular_color);

			shader->setFloat("kA", kA);
			shader->setFloat("kD", kD);
			shader->setFloat("kS", kS);

			shader->setFloat("shininess", shininess);
			shader->setFloat("alpha", alpha);

			shader->setFloat("uv_repeat", uv_repeat);
			shader->setFloat("parallax_heightscale", parallax_heightscale);

			shader->setFloat("detail_diffuse_bias", detail_diffuse_bias);
			shader->setFloat("detail_normal_bias", detail_normal_bias);

			shader->setInt("sample_shadow_map", receive_shadows);

			shader->setInt("sample_diffuse_map", 0);
			shader->setInt("sample_normal_map", 0);
			shader->setInt("sample_displacement_map", 0);
			shader->setInt("sample_detail_diffuse_map", 0);
			shader->setInt("sample_detail_normal_map", 0);

			if (diffuse_map)
			{
				// activate a texture unit per map
				glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEX_UNIT);
				diffuse_map->bind();
				shader->setInt("diffuse_map", DIFFUSE_TEX_UNIT);
				shader->setInt("sample_diffuse_map", 1);
			}

			if (normal_map)
			{
				glActiveTexture(GL_TEXTURE0 + NORMAL_TEX_UNIT);
				normal_map->bind();
				shader->setInt("normal_map", NORMAL_TEX_UNIT);
				shader->setInt("sample_normal_map", 1);
			}
			
			if (displacement_map)
			{
				glActiveTexture(GL_TEXTURE0 + DISPLACEMENT_TEX_UNIT);
				displacement_map->bind();
				shader->setInt("displacement_map", DISPLACEMENT_TEX_UNIT); 
				shader->setInt("sample_displacement_map", 1);
			}

			if (detail_diffuse_map)
			{
				glActiveTexture(GL_TEXTURE0 + DETAIL_DIFFUSE_TEX_UNIT);
				detail_diffuse_map->bind();
				shader->setInt("detail_diffuse_map", DETAIL_DIFFUSE_TEX_UNIT);
				shader->setInt("sample_detail_diffuse_map", 1);
			}
			if (detail_normal_map)
			{
				glActiveTexture(GL_TEXTURE0 + DETAIL_NORMAL_TEX_UNIT);
				detail_normal_map->bind();
				shader->setInt("detail_normal_map", DETAIL_NORMAL_TEX_UNIT);
				shader->setInt("sample_detail_normal_map", 1);
			}
		}

		void unbind() const
		{
			if (!shader) { utils::io::error("MATERIAL - Shader not provided"); return; }

			// Mirroring OpenGL state usage, we should have the shader bound at this point
			
			// We need to unbind these if we use the same shader for different materials
			if (diffuse_map)
			{
				shader->setInt("sample_diffuse_map", 0);
				shader->setInt("diffuse_map", 0);
				diffuse_map->unbind();
			}

			if (normal_map)
			{
				shader->setInt("sample_normal_map", 0);
				shader->setInt("normal_map", 0);
				normal_map->unbind();
			}

			if (displacement_map)
			{
				shader->setInt("sample_displacement_map", 0);
				shader->setInt("displacement_map", 0); 
				displacement_map->unbind();
			}

			if (detail_diffuse_map)
			{
				shader->setInt("sample_detail_diffuse_map", 0);
				shader->setInt("detail_diffuse_map", 0);
				detail_diffuse_map->unbind();
			}

			if (detail_normal_map)
			{
				shader->setInt("sample_detail_normal_map", 0);
				shader->setInt("detail_normal_map", 0);
				detail_normal_map->unbind();
			}

			shader->unbind();
		}

	};
}