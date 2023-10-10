#pragma once

#include <vector>
#include <unordered_map>

#include "camera.h"
#include "light.h"
#include "entity.h"

namespace engine::scene
{
	class Scene
	{
	private:
		std::unordered_map<std::string, Entity*> entities;           
		std::vector<std::string> marked_for_removal;
	public:
		Camera* current_camera;
		std::vector<Light*> lights;

		void add_entity(Entity& entity);

		void mark_for_removal(std::string entity_name);

		void remove_marked();

		void update(float deltaTime);

		void draw() const;

		void custom_draw(Shader& shader) const;
	};
}