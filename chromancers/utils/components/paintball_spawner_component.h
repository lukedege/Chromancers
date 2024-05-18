#pragma once

#include "../component.h"
#include "../scene/paintball_spawner.h"

namespace engine::components
{
	// Component that lets an entity generate paintballs given several parameters (projectile amount, speed, spread...)
	class PaintballSpawnerComponent : public Component
	{
	public:
		constexpr static auto COMPONENT_ID = 3;
	private:

	public:
		PaintballSpawner paintball_spawner; // The actual paintball spawner
		bool active = true; // Toggle for activating the spawning mechanism

		PaintballSpawnerComponent(Entity& parent, PhysicsEngine<Entity>& phy_engine, utils::random::generator& rng, 
			Model& paintball_model, Shader& paintball_shader) :
			Component(parent),
			paintball_spawner{phy_engine, rng, paintball_shader}
		{
			paintball_spawner.current_scene = parent.scene_state().current_scene;
			paintball_spawner.paintball_model = &paintball_model;
		}

		void init()
		{
			
		}

		void update(float delta_time)
		{
			paintball_spawner.update(delta_time);
			if (active)
			{
				paintball_spawner.shoot(_parent->world_transform().position(), _parent->world_transform().orientation(), _parent->world_transform().forward());
			}
		}

		int type()
		{
			return COMPONENT_ID;
		}
	};
}