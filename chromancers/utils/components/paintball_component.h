#pragma once 

#include <optional>

#include "../component.h"
#include "../scene/entity.h"
#include "../io.h"

#include "utils/components/paintable_component.h"

namespace engine::components
{
	// Component that makes an entity explode on impact with another entity's rigidbody: if the impacted entity has a paintmap, it will be altered by this component's paint color
	class PaintballComponent : public Component
	{
	private:
		btRigidBody* parent_rb; // Pointer to parent entity rigidbody
		glm::vec4 paint_color;  // Paint color to show and to apply on impact

		glm::vec3 prev_velocity, current_velocity; // Caching the last and current velocities of the paintball for collision resolution

		float lifetime; // Remaining lifetime (in seconds) of the entity: when this reaches zero, the parent entity will be set for destruction
		// This is to avoid paintballs that never hit anything to live undefinitely and tank performance

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
			// Update cached velocities
			prev_velocity = current_velocity;
			current_velocity = physics::to_glm_vec3(parent_rb->getLinearVelocity());
			
			// Update the lifetime each frame and set for destruction if reaches zero
			lifetime -= delta_time;
			if (lifetime <= 0.f)
				expire();

			// Cancel out gravity
			btVector3 gravity = btVector3(0, -9.82f, 0);
			parent_rb->applyCentralForce(-gravity);

			// Emulate offset center of mass and reapply gravity
			// paintballs should have more mass towards the front of the projectile
			btVector3 impulse_location = parent_rb->getWorldTransform().getBasis() * btVector3(0, -0.01f, -0.005f);
			parent_rb->applyForce(gravity, impulse_location);
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

				// We use the previous velocity since the current velocity may have factored in a bounce  
				// in the physics engine before we can destroy the paintball
				glm::vec3 paintball_direction = glm::normalize(prev_velocity); 
				glm::vec3 paintball_lateral = glm::normalize(glm::cross(paintball_direction, glm::vec3{0, 1, 0})); 
				glm::vec3 paintball_up = glm::normalize(glm::cross(paintball_direction, paintball_lateral));
				
				// Create paintspace viewprojection
				float paint_near_plane = 0.05f, paint_far_plane = 5.f, frustum_size = paintball_size.x * 2, distance_bias = 1.f;
				glm::mat4 paintProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, paint_near_plane, paint_far_plane);
				glm::mat4 paintView = glm::lookAt(paintball_position - paintball_direction * distance_bias, paintball_position + paintball_direction, paintball_up);

				// Make the paintable entity aware of the paintball collision and let it update its paintmap
				other_paintable->update_paintmap(paintProjection * paintView, paintball_direction, paint_color);
			}

			// Set for destruction
			expire();
		}
	private:

		// We ask the parent entity scene to mark this entity for destruction at the end of this frame
		void expire()
		{
			auto& scene_state = _parent->scene_state();
			scene_state.current_scene->mark_for_removal(scene_state.entity_id, scene_state.instanced_group_id);
		}
	};
}