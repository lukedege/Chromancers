#pragma once

#include "scene/entity.h"

namespace utils::graphics::opengl // get a better namespace later because this is not strictly opengl related but engine related
{
	class Entity; // we need to know entity exists while compiling

	class Component
	{
	protected:
		Entity* parent;

	public:
		Component(Entity& parent) : parent{ &parent }{}

		virtual void init() = 0;
		virtual void update(float delta_time) = 0;

		virtual void sync() = 0; // usable for components to syncronize from parent when an event happens
	};
}