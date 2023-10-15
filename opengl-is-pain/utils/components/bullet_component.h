#pragma once 

#include "../component.h"
#include "../scene/entity.h"
#include "../io.h"

namespace engine::components
{
	class BulletComponent : public Component
	{
	public:
		constexpr static auto COMPONENT_ID = 1;

		BulletComponent(scene::Entity& parent) :
			Component(parent)
		{}

		void init()
		{

		}

		void update(float delta_time)
		{

		}

		int type()
		{
			return COMPONENT_ID;
		}

		void on_collision(scene::Entity& other, glm::vec3 contact_point, glm::vec3 normal, glm::vec3 impulse) 
		{
			auto& scene_state = parent->scene_state();
			scene_state.current_scene->mark_for_removal(scene_state.id);
			/*
			utils::io::info(parent->display_name, " is colliding with ", other.display_name, " at coords ", glm::to_string(contact_point));
			utils::io::info("Normal is: ", glm::to_string(normal));
			utils::io::info("Impulse is: ", glm::to_string(impulse));
			utils::io::info("--------------------------------------------------------------------------");
			*/
		};
	};
}