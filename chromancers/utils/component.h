#pragma once

#include <glm/glm.hpp>

namespace engine::scene
{ 
	class Entity; // Forward declaration to avoid cyclical include
}

namespace engine::components 
{
	// Base component class for enriching entities of additional and modular behaviours
	class Component
	{
	protected:
		scene::Entity* _parent; // We always keep a pointer to the original owner of the component

	public:
		Component(scene::Entity& parent) : _parent{ &parent }{}
		virtual ~Component() {}
		
		virtual int type() = 0; // The type of the component helps finding it in collections

		virtual void init() = 0; 
		virtual void update(float delta_time) = 0;

		// Callback for various events that could be triggered by the parent entity
		virtual void on_transform_update() {}; 
		virtual void on_collision(scene::Entity& other, glm::vec3 contact_point, glm::vec3 normal, glm::vec3 impulse) {};

		const scene::Entity* parent()
		{
			return _parent;
		}
	};
}