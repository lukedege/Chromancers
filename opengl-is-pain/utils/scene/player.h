#pragma once

#include "camera.h"
#include "entity.h"
#include "utils/physics.h"

#include "utils/components/rigidbody_component.h"
#include "utils/components/paintball_component.h"

#include <glm/gtx/vector_angle.hpp>

namespace
{
	using namespace engine::physics;
	using namespace engine::components;
}

namespace engine::scene
{
	class Player
	{
	public:
		EntityBase player_entity;
		Entity* gun_entity;
		Scene* current_scene;
		Model* bullet_model;
		Material* bullet_material;

		Camera first_person_camera;		
		float lerp_speed = 10.f;
		
		glm::vec4 paint_color{ 1.f, 0.85f, 0.f, 1.f };
		
		float paintball_weight { 1.f };
		float paintball_size   { 0.1f };
		float size_variation_min_multiplier{ 1.f };
		float size_variation_max_multiplier{ 1.f };

		unsigned int rounds_per_second{ 100 };
		float muzzle_speed  { 20.f };
		float muzzle_spread { 1.5f };
		
		glm::vec3 viewmodel_offset{ 0 };

	private:
		float fire_cooldown_timer { 1.f/rounds_per_second };

	public:


		Player() :
			player_entity{"PlayerObject"},
			gun_entity{nullptr},
			first_person_camera{ glm::vec3{0,0,5} }
		{

		}

		void init()
		{
			gun_entity->init();

			if (gun_entity)
			{
				gun_entity->parent = &player_entity;
				gun_entity->rotate(glm::vec3{0, 90.f, 0});
			}
			
			player_entity.set_position(first_person_camera.position());
			player_entity.set_orientation({ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z});
		}

		void update(float delta_time)
		{	
			sync_to_cam(delta_time);
			gun_entity->update(delta_time);

			// we freeze the value to avoid underflows
			if (fire_cooldown_timer <= 0) { fire_cooldown_timer = 0; }
			else { fire_cooldown_timer -= delta_time; }
		}

		void draw()
		{
			gun_entity->draw();
		}

		// Creates and launches a paintball, if precise is true then there will be no spread among fired bullets 
		void shoot(bool precise, PhysicsEngine<Entity>& physics_engine) // TODO temp parameters
		{
			if (gun_entity && current_scene && fire_cooldown_timer <= 0)
			{
				auto& rng = current_scene->rng; // TODO temp

				if (bullet_model && bullet_material) // TODO temp, make a better check
				{
					Entity* bullet = current_scene->emplace_instanced_entity("bullets", "bullet", "This is a paintball", *bullet_model, *bullet_material);

					bullet->set_position(gun_muzzle_world_position() + gun_entity->world_transform().forward() * paintball_size * 2.f);
					glm::vec3 ori = player_entity.world_transform().orientation();
					glm::vec3 bullet_orientation = glm::vec3{ ori.z, ori.y, ori.x } - glm::vec3{0, 90.f, 0};
					bullet->set_orientation(bullet_orientation);

					glm::vec3 paintball_final_size = glm::vec3(paintball_size) * rng.get_float(size_variation_min_multiplier, size_variation_max_multiplier);
					bullet->set_size(paintball_final_size); 

					// setting up collision mask for paintball rigidbodies (to avoid colliding with themselves)
					CollisionFilter paintball_cf{ 1 << 7, ~(1 << 7) }; // this means "Paintball group is 1 << 7 (128), mask is everything but paintball group (inverse of the group bit)"

					// create compound collision shape for bullet
					bullet->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ paintball_weight, 0.1f, 0.1f, {ColliderShape::BOX, glm::vec3{paintball_size, paintball_size, paintball_size*2}} }, paintball_cf, false);
					bullet->emplace_component<PaintballComponent>(paint_color);

					bullet->init();

					float speed_modifier = muzzle_speed;
					glm::vec3 spread_modifier {0};

					if (!precise)
					{
						float speed_bias = 2.0f;
						float spread_bias = muzzle_spread;
						float speed_spread = rng.get_float(-1, 1) * speed_bias; // get float between -bias and +bias
						speed_modifier += speed_spread;
						spread_modifier = normalize(glm::vec3{rng.get_float(-1, 1), rng.get_float(-1, 1), rng.get_float(-1, 1)}) * spread_bias;
					}

					glm::vec3 shoot_dir = gun_entity->world_transform().forward() * speed_modifier + spread_modifier;
					btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
					bullet->get_component<RigidBodyComponent>()->rigid_body->applyCentralImpulse(impulse);
				}
				fire_cooldown_timer = 1.f/rounds_per_second;
			}
		}

		glm::vec3 gun_muzzle_world_position()
		{
			glm::mat4 gun_world_transform = gun_entity ? gun_entity->world_transform().matrix() : glm::mat4{1.f};
			return glm::vec3 { gun_world_transform * glm::vec4 { local_muzzle_position, 1.f } };
		}

	private:
		glm::vec3 local_muzzle_position { 0.0f, 0.1f, 0.67f };
		
		void sync_to_cam(float delta_time)
		{
			float f_ang = std::clamp(lerp_speed * delta_time, 0.f, 1.f);

			player_entity.set_position(first_person_camera.position());
			player_entity.set_orientation(glm::mix(player_entity.world_transform().orientation(), { first_person_camera.rotation().x, -first_person_camera.rotation().y, first_person_camera.rotation().z}, f_ang));

			if (gun_entity)
			{
				gun_entity->set_position(viewmodel_offset);
			}
		}
	};
}