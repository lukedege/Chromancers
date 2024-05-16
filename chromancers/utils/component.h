#pragma once

#include <glm/glm.hpp>

namespace engine::scene
{ 
	class Entity; // Forward declaration to avoid cyclical include
}

namespace engine::components 
{
	class Component
	{
	protected:
		scene::Entity* _parent;

	public:
		Component(scene::Entity& parent) : _parent{ &parent }{}
		virtual ~Component() {}

		virtual void init() = 0;
		virtual void update(float delta_time) = 0;
		virtual int type() = 0;

		virtual void on_transform_update() {};
		virtual void on_collision(scene::Entity& other, glm::vec3 contact_point, glm::vec3 normal, glm::vec3 impulse) {};

		const scene::Entity* parent()
		{
			return _parent;
		}
	};
}