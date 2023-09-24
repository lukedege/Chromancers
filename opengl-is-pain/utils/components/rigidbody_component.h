#pragma once

#include "../component.h"
#include "../physics.h"

// unnamed namespace will keep this namespace declaration for this file only, even if included
namespace
{
	using namespace utils::physics;
}

namespace utils::graphics::opengl
{
	class RigidBodyComponent : public Component
	{
		
	private:
		PhysicsEngine* physics_engine;

	public:
		btRigidBody* rigid_body;
		bool is_kinematic; // TODO check if rigid body's mass needs to be set to zero or this is enough

		RigidBodyComponent(Entity& parent, PhysicsEngine& phy_engine, PhysicsEngine::RigidBodyCreateInfo rb_cinfo) :
			Component(parent),
			physics_engine{&phy_engine},
			rigid_body { physics_engine->createRigidBody(parent.transform.position(), parent.transform.size(), parent.transform.orientation(), rb_cinfo) },
			is_kinematic {false}
		{}

		~RigidBodyComponent()
		{
			physics_engine->deleteRigidBody(rigid_body);
			rigid_body = nullptr;
		}

		void init()
		{
			
		}

		void update(float delta_time)
		{
			if (!is_kinematic)
			{
				GLfloat matrix[16];
				btTransform bt_transform;
				glm::vec3 size = parent->transform.size();

				// we take the transformation matrix of the rigid boby, as calculated by the physics engine
				rigid_body->getMotionState()->getWorldTransform(bt_transform);

				// we convert the Bullet matrix (transform) to an array of floats
				bt_transform.getOpenGLMatrix(matrix);

				// Bullet matrix provides rotations and translations: it does not consider scale
				parent->transform.set(glm::make_mat4(matrix) * glm::scale(glm::mat4{ 1.0f }, size));
			}
		}

		// Syncs physics position with parent entity transform
		void sync()
		{
			reset_position(parent->transform.position());
		}

	private:
		void reset_position(const glm::vec3 new_position)
		{
			// Copying old transform and changing the position
			btTransform old_physics_transform = rigid_body->getWorldTransform();
			btVector3 new_pos{ new_position.x, new_position.y, new_position.z };
			old_physics_transform.setOrigin(new_pos);

			// Updating physics transforms
			rigid_body->setWorldTransform(old_physics_transform);
			rigid_body->getMotionState()->setWorldTransform(old_physics_transform);

			// Resetting forces and velocities
			rigid_body->setLinearVelocity(btVector3{ 0,0,0 });
			rigid_body->setAngularVelocity(btVector3{ 0,0,0 });
			rigid_body->clearForces();

			rigid_body->activate();
		}
	};
}