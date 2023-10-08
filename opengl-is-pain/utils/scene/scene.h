#pragma once

#include <vector>

#include "camera.h"
#include "light.h"
#include "entity.h"

namespace engine::scene
{
	class Scene
	{
	private:
		
	public:
		std::vector<Entity*> entities;  // TODO temporarily public //TODO2 for now vector is sufficient, a tree or another container (e.g. unordered_map) would be better 
		Camera* current_camera;
		std::vector<Light*> lights;

		void add_entity(Entity& entity)
		{
			entities.push_back(&entity);
		}

		void update(float deltaTime)
		{
			for (Entity* o : entities)
			{
				o->update(deltaTime);
			}
		}

		void draw() const
		{
			for (Entity* o : entities)
			{
				o->draw();
			}
		}

		void custom_draw(Shader& shader) const
		{
			for (Entity* o : entities)
			{
				o->custom_draw(shader);
			}
		}
	
		// TODO
	};
}