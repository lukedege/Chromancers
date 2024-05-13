#pragma once

#include "camera.h"
#include "entity.h"
#include "../physics.h"
#include "../random.h"

#include "../components/rigidbody_component.h"
#include "../components/paintball_component.h"

#include <glm/gtx/vector_angle.hpp>

namespace
{
	using namespace engine::physics;
	using namespace engine::components;
}

namespace engine::scene
{
	class PaintballSpawner
	{
	public:
		Scene*     current_scene {nullptr};
		PhysicsEngine<Entity>& physics_engine;
		utils::random::generator& rng;

		Model*    paintball_model    {nullptr};
		Material  paintball_material;

		glm::vec4 paint_color{ 1.f, 0.85f, 0.f, 1.f };
		
		float paintball_weight { 1.f };
		float paintball_size   { 0.1f };
		float size_variation_min_multiplier { 1.f };
		float size_variation_max_multiplier { 1.f };

		unsigned int rounds_per_second{ 100 };
		float shooting_speed  { 20.f };
		float shooting_spread { 1.5f };
	private:
		float fire_cooldown_timer { 1.f / rounds_per_second };
	public:
		PaintballSpawner(PhysicsEngine<Entity>& physics_engine, utils::random::generator& rng, Shader& paintball_shader) :
			physics_engine{physics_engine},
			rng{rng},
			paintball_material{paintball_shader}
		{
		}

		void update(float delta_time)
		{
			paintball_material.diffuse_color = paint_color; 
			paintball_material.ambient_color = paint_color;

			// we freeze the value to avoid underflows
			if (fire_cooldown_timer <= 0) { fire_cooldown_timer = 0; }
			else { fire_cooldown_timer -= delta_time; }
		}

		void shoot(glm::vec3 spawn_position, glm::vec3 spawn_orientation, glm::vec3 shoot_direction)
		{
			if (fire_cooldown_timer <= 0)
			{
				shoot_pb(spawn_position, spawn_orientation, shoot_direction);

				// reset cooldown after shooting
				fire_cooldown_timer = 1.f/rounds_per_second;
			}
		}

		void shoot_pb(glm::vec3 spawn_position, glm::vec3 spawn_orientation, glm::vec3 shoot_direction)
		{
			std::string this_spawner_address = std::to_string((unsigned long long)(void**)this);
			std::string group_instance_id = "paintballs" + this_spawner_address;
			Entity* paintball = current_scene->emplace_instanced_entity(group_instance_id, "paintball", "This is a paintball", *paintball_model, paintball_material);

			paintball->set_position(spawn_position);
			paintball->set_orientation(spawn_orientation);

			glm::vec3 paintball_final_size = glm::vec3(paintball_size) * rng.get_float(size_variation_min_multiplier, size_variation_max_multiplier);
			paintball->set_size(paintball_final_size); 

			// setting up collision mask for paintball rigidbodies (to avoid colliding with themselves)
			CollisionFilter paintball_cf{ 1 << 7, ~(1 << 7) }; // this means "Paintball group is 1 << 7 (128), mask is everything but paintball group (inverse of the group bit)"

			// add the related components
			paintball->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ paintball_weight, 0.1f, 0.1f, 
				ColliderShapeCreateInfo{ ColliderShape::BOX, glm::vec3{paintball_final_size.x, paintball_final_size.y, paintball_final_size.z*2}} }, paintball_cf, false);
			paintball->emplace_component<PaintballComponent>(paint_color);

			paintball->init();

			// calculate speed and spread modifiers
			float speed_modifier = shooting_speed;
			glm::vec3 spread_modifier {0};

			float speed_bias = 2.0f;
			float spread_bias = shooting_spread;
			//float speed_spread = rng.get_float(-1, 1) * speed_bias; // get float between -bias and +bias
			spread_modifier = normalize(glm::vec3{rng.get_float(-1, 1), rng.get_float(-1, 1), rng.get_float(-1, 1)}) * spread_bias;

			// calculate and apply impulse to paintball
			glm::vec3 shoot_dir = shoot_direction * speed_modifier + spread_modifier;
			btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
			paintball->get_component<RigidBodyComponent>()->rigid_body->applyCentralImpulse(impulse);
		}
	};
}