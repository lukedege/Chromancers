#pragma once

namespace engine::scene
{ 
	class Entity; // Forward declaration to avoid cyclical include
}

namespace engine::components 
{
	class Component
	{
	protected:
		scene::Entity* parent;

	public:
		Component(scene::Entity& parent) : parent{ &parent }{}
		~Component() { parent = nullptr; }

		virtual void init() = 0;
		virtual void update(float delta_time) = 0;

		virtual void on_transform_update() {};
		virtual void on_collision(scene::Entity& other, glm::vec3 contact_point) {};
	};
}