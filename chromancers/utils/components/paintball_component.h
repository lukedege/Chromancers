#pragma once 

#include <optional>

#include "../component.h"
#include "../scene/entity.h"
#include "../io.h"

#include "utils/components/paintable_component.h"

namespace engine::components
{
	class PaintballComponent : public Component
	{
		btRigidBody* parent_rb;
		glm::vec4 paint_color;
		glm::vec3 prev_velocity, current_velocity; // we need this as current velocity when collision happens factors in the bounce...
		float lifetime; // i think its seconds
	public:
		constexpr static auto COMPONENT_ID = 1;

		PaintballComponent(scene::Entity& parent, glm::vec4 paint_color) :
			Component(parent),
			paint_color { paint_color },
			prev_velocity    { 0 },
			current_velocity { 0 },
			lifetime { 7.f }
		{}

		void init()
		{
			parent_rb = _parent->get_component<RigidBodyComponent>()->rigid_body;
			if (!parent_rb) utils::io::warn("PaintballComponent - RigidBody component not found: a RigidBody component is required for this component to work.");
		}

		void update(float delta_time)
		{
			prev_velocity = current_velocity;
			current_velocity = physics::to_glm_vec3(parent_rb->getLinearVelocity());
			
			lifetime -= delta_time;
			if (lifetime <= 0.f)
				expire();

			// cancel out gravity
			btVector3 gravity = btVector3(0, -9.82f, 0);
			parent_rb->applyCentralForce(-gravity);

			// emulate offset center of mass
			btVector3 imploc = parent_rb->getWorldTransform().getBasis() * btVector3(0, -0.01f, -0.005f);
			parent_rb->applyForce(gravity, imploc);
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
				glm::vec3 paintball_size = _parent->world_transform().size();
				glm::vec3 paintball_position = _parent->world_transform().position();
				glm::vec3 paintball_direction = glm::normalize(prev_velocity);
				glm::vec3 paintball_lateral = glm::normalize(glm::cross(paintball_direction, glm::vec3{0, 1, 0})); 
				glm::vec3 paintball_up = glm::normalize(glm::cross(paintball_direction, paintball_lateral));
				
				// Create paintspace viewprojection
				float paint_near_plane = 0.05f, paint_far_plane = 5.f, frustum_size = paintball_size.x * 2, distance_bias = 1.f;
				glm::mat4 paintProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, paint_near_plane, paint_far_plane);
				glm::mat4 paintView = glm::lookAt(paintball_position - paintball_direction * distance_bias, paintball_position + paintball_direction, paintball_up);

				other_paintable->update_paintmap(paintProjection * paintView, paintball_direction, paint_color);
			}

			// Set for destruction
			expire();

			/*
			utils::io::info(parent->display_name, " is colliding with ", other.display_name, " at coords ", glm::to_string(contact_point));
			utils::io::info("Normal is: ", glm::to_string(normal));
			utils::io::info("Impulse is: ", glm::to_string(impulse));
			utils::io::info("--------------------------------------------------------------------------");
			*/
		}
	private:
		void expire()
		{
			auto& scene_state = _parent->scene_state();
			scene_state.current_scene->mark_for_removal(scene_state.entity_id, scene_state.instanced_group_id);
		}
	};
}