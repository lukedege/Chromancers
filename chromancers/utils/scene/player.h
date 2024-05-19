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
	// Class representing the player's main way to observe and interact with the scene
	class Player
	{
	public:
		EntityBase player_entity;           // Main entity that will be synced to the camera (since it's the one receiving movement input)
		Entity*    gun_entity    {nullptr}; // Gun entity that will be attached to the main player entity

		PaintballSpawner* paintball_spawner{ nullptr }; // Paintball spawner that simulates the gun paintball chamber

		Camera first_person_camera;	// First person camera moved by user input
		float lerp_speed = 10.f;    // Interpolation speed to make gun movement feel more natural while syncing to the camera
		
		glm::vec3 viewmodel_offset{ 0 }; // Offset of the gun model from the default position (e.g. CSGO viewmodel_offset)

		Player() :
			player_entity{"PlayerObject"},
			first_person_camera{ glm::vec3{0,0,10} }
		{
			first_person_camera.set_planes(.1f, 100.f);
		}

		void init()
		{
			gun_entity->init();

			// The gun will be child of the main player entity, so that it moves together with it
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

		// Invoke the paintball spawner [ in the gun :) ] to generate and shoot a paintball
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

		// Calculates and returns the world position of the gun muzzle
		glm::vec3 gun_muzzle_world_position()
		{
			glm::mat4 gun_world_transform = gun_entity ? gun_entity->world_transform().matrix() : glm::mat4{1.f};
			return glm::vec3 { gun_world_transform * glm::vec4 { local_muzzle_position, 1.f } };
		}

	private:
		glm::vec3 local_muzzle_position { 0.0f, 0.1f, 0.67f }; // This depends on the gun model
		
		// Synchronize the player entity to the camera movement
		void sync_to_cam(float delta_time)
		{
			float f_ang = std::clamp(lerp_speed * delta_time, 0.f, 1.f);

			player_entity.set_position(first_person_camera.position());
			player_entity.set_orientation(glm::mix(player_entity.world_transform().orientation(), { first_person_camera.rotation().x, -first_person_camera.rotation().y, first_person_camera.rotation().z}, f_ang));

			if (gun_entity)
			{
				gun_entity->set_position(viewmodel_offset); // Update offset in case it was modified in the UI by the user
			}
		}

	};
}