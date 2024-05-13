#pragma once

#include "camera.h"
#include "entity.h"
#include "paintball_spawner.h"

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
	class Player
	{
	public:
		EntityBase player_entity;
		Entity*    gun_entity    {nullptr};
		PaintballSpawner* paintball_spawner{ nullptr };

		Camera first_person_camera;		
		float lerp_speed = 10.f;
		
		glm::vec3 viewmodel_offset{ 0 };

		Player() :
			player_entity{"PlayerObject"},
			first_person_camera{ glm::vec3{0,0,5} }
		{}

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
			paintball_spawner->update(delta_time);
		}

		void draw()
		{
			gun_entity->draw();
		}

		// Creates and launches a paintball
		void shoot()
		{
			if (gun_entity && paintball_spawner)
			{
				glm::vec3 spawn_position    = gun_muzzle_world_position() + gun_entity->world_transform().forward() * paintball_spawner->paintball_size * 2.f;

				glm::vec3 ori = player_entity.world_transform().orientation();
				glm::vec3 spawn_orientation = glm::vec3{ ori.z, ori.y, ori.x } - glm::vec3{0, 90.f, 0};

				glm::vec3 shoot_dir = gun_entity->world_transform().forward();
				paintball_spawner->shoot(spawn_position, spawn_orientation, shoot_dir);
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