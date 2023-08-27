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

		Material(Shader& shader, Texture* diffuse = nullptr, Texture* normal = nullptr, Texture* disp = nullptr) :
			shader          { &shader },
			diffuse_map     { diffuse },
			normal_map      { normal  },
			displacement_map{ disp    }
		{

		}

		void use()
		{
			shader->use();
			
			if (diffuse_map)
			{
				diffuse_map->activate();
				shader->setInt("diffuse_tex", diffuse_map->id);
			}

			if (normal_map)
			{
				normal_map->activate();
				shader->setInt("normal_tex", normal_map->id);
			}
			
			if (displacement_map)
			{
				displacement_map->activate();
				shader->setInt("depth_tex", displacement_map->id); // TODO choose a single f*ing name for displacement textures (displacement? depth? height???? choose ONE)
			}
		}
	};
}