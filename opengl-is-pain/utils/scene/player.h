#pragma once

#include "camera.h"
#include "entity.h"

#include <glm/gtx/vector_angle.hpp>

namespace engine::scene
{
	class Player
	{
	public:
		EntityBase player_entity;
		Entity* gun_entity;

		Camera first_person_camera;

		glm::vec3 viewmodel_offset{ 0 };
		
		float lerp_speed = 10.f;
		//Camera third_person_camera;

		Player() :
			player_entity{"PlayerObject"},
			gun_entity{nullptr},
			first_person_camera{ glm::vec3{0,0,5} }
		{

		}

		void init()
		{
			if (gun_entity)
			{
				gun_entity->parent = &player_entity;
				gun_entity->rotate(glm::vec3{0, 90.f, 0});
			}
			
			player_entity.set_position(first_person_camera.position());
			player_entity.set_rotation({ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z});
		}

		void update(float delta_time)
		{	
			sync_to_cam(delta_time);
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
			
			//float dist = glm::distance(player_entity.world_transform().position(), first_person_camera.position());
			//float f_pos = std::clamp(((dist+0.1f) + (1/(dist+0.1f)))  * 8 * delta_time, 0.f, 1.f);
			//player_entity.set_position(glm::mix(player_entity.world_transform().position(), first_person_camera.position(), f_pos));
			
			//glm::vec3 norm_p_orient = glm::normalize(player_entity.world_transform().orientation());
			//glm::vec3 norm_c_orient = glm::normalize(glm::vec3{ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z });
			//float angle_dist = glm::angle(norm_p_orient, norm_c_orient);

			float f_ang = std::clamp(lerp_speed * delta_time, 0.f, 1.f);

			player_entity.set_position(first_person_camera.position());
			player_entity.set_rotation(glm::mix(player_entity.world_transform().orientation(), { 0, -first_person_camera.rotation().y, first_person_camera.rotation().z}, f_ang));

			if (gun_entity)
			{
				gun_entity->set_position(viewmodel_offset);
			}

			//utils::io::info("player pos", glm::to_string(player_entity.local_transform().position()), " ", glm::to_string(player_entity.world_transform().position()));
			//utils::io::info("gun pos", glm::to_string(gun->local_transform().position()), " ", glm::to_string(gun->world_transform().position()));
			//utils::io::info("cam pos", glm::to_string(first_person_camera.position()), " ", glm::to_string(first_person_camera.position()));
		}
	};
}