#pragma once

#include <vector>
#include "camera.h"
#include "light.h"
#include "../physics.h"

namespace utils::graphics::opengl
{
	struct SceneData
	{
		Camera* current_camera;
		std::vector<Light*> lights;
		physics::PhysicsEngine* physics_engine;
	};

	/*
	class Scene
	{
	private:
		std::vector<Entity*> entities;  //TODO for now vector is sufficient, a tree or a better container (e.g. unordered_map) would be optimal 
	public:
		Camera* current_camera;
		std::vector<Light*> lights;
		physics::PhysicsEngine* physics_engine;

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

		void draw()
		{
			for (Entity* o : entities)
			{
				o->draw();
			}
		}
	
		// TODO
	};*/
}