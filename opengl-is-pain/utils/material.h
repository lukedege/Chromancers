#pragma once

#include <memory>

#include "shader.h"
#include "texture.h"

namespace utils::graphics::opengl
{
	class Material
	{
	public:
		Shader* shader;
		Texture* diffuse_map;
		Texture* normal_map;
		Texture* displacement_map;

		glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
		float kD = 0.5f, kS = 0.4f, kA = 0.1f; // Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
		float shininess = 25.f;
		float alpha = 0.2f;
		float F0 = 0.9f;

		Material(Shader& shader, Texture* diffuse = nullptr, Texture* normal = nullptr, Texture* disp = nullptr) :
			shader          { &shader },
			diffuse_map     { diffuse },
			normal_map      { normal  },
			displacement_map{ disp    }
		{

		}

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
				shader->setInt("depth_tex", displacement_map->id); // TODO choose a single f*ing name for displacement textures (displacement? depth? height???? choose ONE)
			}
		}

		void unbind()
		{
			// TODO
		}

	};
}