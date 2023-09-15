#pragma once

#include <memory>

#include "shader.h"
#include "texture.h"

namespace utils::graphics::opengl
{
	class Material
	{
		using Color = glm::vec4;
	public:
		Shader* shader;
		Color albedo              { 1 };
		Texture* diffuse_map      { nullptr };
		Texture* normal_map       { nullptr };
		Texture* displacement_map { nullptr };

		// "Fake" lighting parameters
		float kA{ 0.1f }, kD{ 0.5f }, kS{ 0.4f };
		glm::vec3 ambient { 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
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

			shader->setVec3("ambient", ambient);
			shader->setVec3("diffuse", diffuse);
			shader->setVec3("specular", specular);

			shader->setFloat("kA", kA);
			shader->setFloat("kD", kD);
			shader->setFloat("kS", kS);

			shader->setFloat("shininess", shininess);
			shader->setFloat("alpha", alpha);

			shader->setFloat("uv_repeat", uv_repeat);
			shader->setFloat("parallax_heightscale", parallax_heightscale);
			
			if (diffuse_map)
			{
				diffuse_map->bind();
				shader->setInt("diffuse_tex", diffuse_map->id);
			}

			if (normal_map)
			{
				normal_map->bind();
				shader->setInt("normal_tex", normal_map->id);
			}
			
			if (displacement_map)
			{
				displacement_map->bind();
				shader->setInt("displacement_tex", displacement_map->id); // TODO choose a single f*ing name for displacement textures (displacement? depth? height???? choose ONE)
			}
		}

		void unbind()
		{
			shader->unbind();
			glBindTexture(GL_TEXTURE_2D, 0); // unbinds any texture used before, if any
		}

	};
}