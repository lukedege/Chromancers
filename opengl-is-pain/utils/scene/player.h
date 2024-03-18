#pragma once

#include "camera.h"
#include "entity.h"

namespace engine::scene
{
	struct Player
	{
		Camera first_person_camera;
		Entity* gun;

		glm::vec3 viewmodel_offset{ 0 };
		//Camera third_person_camera;

		void init()
		{
			//gun->rotate(glm::vec3{0, 180.f, 0});
		}

		void update()
		{	
			gun->set_position(first_person_camera.position() + viewmodel_offset);
		}
	};
}