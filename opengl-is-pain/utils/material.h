#pragma once

#include <memory>

#include "shader.h"
#include "texture.h"

namespace engine::resources
{
	class Material
	{
		using Color = glm::vec4;
	public:
		Shader* shader;
		//Color albedo              { 1 }; TODO use in place of diffuse when ambient and specular are gone
		Texture* diffuse_map      { nullptr };
		Texture* normal_map       { nullptr };
		Texture* displacement_map { nullptr };

		// "Fake" lighting parameters
		float kA{ 0.1f }, kD{ 0.5f }, kS{ 0.4f };
		Color ambient_color { 0.1f, 0.1f, 0.1f, 1.f }, diffuse_color{ 1.0f, 1.0f, 1.0f, 1.f }, specular_color{ 1.0f, 1.0f, 1.0f, 1.f };
		float shininess   { 25.f };

		// SchlickGGX parameters
		float alpha { 0.2f }; // rugosity - 0 : smooth, 1: rough
		float F0    { 0.9f }; // fresnel reflectance at normal incidence

		// Texture parameters
		float uv_repeat { 3.f };

		// Height map parameters
		float parallax_heightscale { 0.05f };

		Material(Shader& shader) : shader { &shader } {}

		// TODO are setters really necessary?
		//void set_diffuse_map     (Texture& diff_map) { diffuse_map      = &diff_map; }
		//void set_normal_map      (Texture& norm_map) { normal_map       = &norm_map; }
		//void set_displacement_map(Texture& disp_map) { displacement_map = &disp_map; }

		void bind()
		{
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
			
			if (diffuse_map)
			{
				// activate a texture unit per map
				glActiveTexture(GL_TEXTURE0);
				diffuse_map->bind();
				shader->setInt("diffuse_tex", 0);
				shader->setInt("sample_diffuse_map", 1);
			}

			if (normal_map)
			{
				glActiveTexture(GL_TEXTURE1);
				normal_map->bind();
				shader->setInt("normal_tex", 1);
				shader->setInt("sample_normal_map", 1);
			}
			
			if (displacement_map)
			{
				glActiveTexture(GL_TEXTURE2);
				displacement_map->bind();
				shader->setInt("displacement_tex", 2); 
				shader->setInt("sample_displacement_map", 1);
			}
		}

		void unbind()
		{
			// We need to unbind these if we use the same shader for different materials
			if (diffuse_map)
			{
				shader->setInt("sample_diffuse_map", 0);
				shader->setInt("diffuse_tex", 0);
				diffuse_map->unbind();
			}

			if (normal_map)
			{
				shader->setInt("sample_normal_map", 0);
				shader->setInt("normal_tex", 0);
				normal_map->unbind();
			}

			if (displacement_map)
			{
				shader->setInt("sample_displacement_map", 0);
				shader->setInt("displacement_tex", 0); 
				displacement_map->unbind();
			}
			shader->unbind();
		}

	};
}