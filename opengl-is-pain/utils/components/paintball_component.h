#pragma once 

#include "../component.h"
#include "../scene/entity.h"
#include "../io.h"

namespace engine::components
{
	class PaintballComponent : public Component
	{
		btRigidBody* parent_rb;
		glm::vec3 prev_velocity, current_velocity; // we need this as current velocity when collision happens factors in the bounce...
	public:
		constexpr static auto COMPONENT_ID = 1;

		PaintballComponent(scene::Entity& parent) :
			Component(parent),
			prev_velocity    { 0 },
			current_velocity { 0 }
		{}

		void init()
		{
			parent_rb = parent->get_component<RigidBodyComponent>()->rigid_body;
			if (!parent_rb) utils::io::warn("BULLET - RigidBody component not found. Something went wrong...");
		}

		void update(float delta_time)
		{
			prev_velocity = current_velocity;
			current_velocity = physics::to_glm_vec3(parent_rb->getLinearVelocity());
			utils::io::info("Curr. lin. vel. : ", current_velocity.x, ", ", current_velocity.y, ", ", current_velocity.z);
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
				glm::vec3 bullet_position = parent->transform().position();
				glm::vec3 bullet_direction = glm::normalize(prev_velocity);
				glm::vec3 bullet_lateral = glm::normalize(glm::cross(bullet_direction, glm::vec3{0, 1, 0})); 
				glm::vec3 bullet_up = glm::normalize(glm::cross(bullet_direction, bullet_lateral));
				
				// Create paintspace viewprojection
				float paint_near_plane = 0.05f, paint_far_plane = 2.f, frustum_size = 1.f, distance_bias = 1.f;
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