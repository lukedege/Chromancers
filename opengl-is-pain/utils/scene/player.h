#pragma once

#include "camera.h"
#include "entity.h"

namespace engine::scene
{
	struct Player
	{
		
		EntityBase player_entity;
		Entity* gun;

		Camera first_person_camera;
		

		glm::vec3 viewmodel_offset{ 0 };
		float lerp_speed = 50.f;
		//Camera third_person_camera;

		Player() :
			player_entity{"PlayerObject"},
			gun{nullptr},
			first_person_camera{ glm::vec3{0,0,5} }
		{

		}

		void init()
		{
			gun->parent = &player_entity;
			gun->rotate(glm::vec3{0, 90.f, 0});
		}

		void update(float delta_time)
		{	
			gun->set_position(viewmodel_offset);
			
			player_entity.set_position(glm::mix(player_entity.world_transform().position(), first_person_camera.position(), lerp_speed*delta_time));
			player_entity.set_rotation({ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z});
			//utils::io::info("player pos", glm::to_string(player_entity.local_transform().position()), " ", glm::to_string(player_entity.world_transform().position()));
			//utils::io::info("gun pos", glm::to_string(gun->local_transform().position()), " ", glm::to_string(gun->world_transform().position()));
		}
	};
}