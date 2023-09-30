#pragma once

#include "scene/entity.h"

namespace engine::scene
{
	class Entity;
}

namespace engine::components // get a better namespace later because this is not strictly opengl related but engine related
{
	class Component
	{
	protected:
		Entity* parent;

	public:
		Component(Entity& parent) : parent{ &parent }{}
		~Component() { parent = nullptr; }

		virtual void init() = 0;
		virtual void update(float delta_time) = 0;

		virtual void on_transform_update() {}; // usable for components to syncronize from parent when an event happens
	};
}