#pragma once 

#include "../component.h"
#include "../scene/entity.h"
#include "../io.h"

namespace engine::components
{
	class PaintballComponent : public Component
	{
	public:
		constexpr static auto COMPONENT_ID = 1;

		PaintballComponent(scene::Entity& parent) :
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
			PaintableComponent* other_paintable = other.get_component<PaintableComponent>();

			if (other_paintable)
			{
				RigidBodyComponent* parent_rb = parent->get_component<RigidBodyComponent>();

				glm::vec3 bullet_direction {0};
				glm::vec3 bullet_up {0};
				glm::vec3 bullet_position = parent->transform().position();
			
				if (parent_rb) { 
					bullet_direction = glm::normalize(physics::to_glm_vec3(parent_rb->rigid_body->getLinearVelocity()));
					glm::vec3 bullet_lateral = glm::normalize(glm::cross(bullet_direction, glm::vec3{0, 1, 0})); 
					bullet_up = glm::normalize(glm::cross(bullet_direction, bullet_lateral));
				}
				else { utils::io::warn("BULLET - RigidBody component not found. Something went wrong..."); } // TODO temp

				// Create paintspace viewprojection
				float paint_near_plane = 0.05f, paint_far_plane = 3.f, frustum_size = 1.f, distance_bias = 2.f;
				glm::mat4 paintProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, paint_near_plane, paint_far_plane);
				glm::mat4 paintView = glm::lookAt(bullet_position - bullet_direction * distance_bias, bullet_position + bullet_direction, bullet_up);

				other_paintable->update_paintmap(paintProjection * paintView, bullet_direction);
			}

			// Set for destruction
			auto& scene_state = parent->scene_state();
			scene_state.current_scene->mark_for_removal(scene_state.entity_id);

			/*
			utils::io::info(parent->display_name, " is colliding with ", other.display_name, " at coords ", glm::to_string(contact_point));
			utils::io::info("Normal is: ", glm::to_string(normal));
			utils::io::info("Impulse is: ", glm::to_string(impulse));
			utils::io::info("--------------------------------------------------------------------------");
			*/
		};
	};
}