#pragma once

#include "../component.h"
#include "../physics.h"
#include "../transform.h"

namespace engine::components
{
	class RigidBodyComponent : public Component
	{
		using PhysicsEngine = engine::physics::PhysicsEngine<Entity>;
		using RigidBodyCreateInfo = engine::physics::RigidBodyCreateInfo;
		using CollisionFilter = engine::physics::CollisionFilter;

	public:
		constexpr static auto COMPONENT_ID = 0;
	private:
		PhysicsEngine* physics_engine;

	public:
		btRigidBody* rigid_body;
		bool is_kinematic; // TODO check if rigid body's mass needs to be set to zero or this is enough

		RigidBodyComponent(Entity& parent, PhysicsEngine& phy_engine, RigidBodyCreateInfo rb_cinfo, bool use_transform_size = false) :
			Component(parent),
			physics_engine{&phy_engine},
			rigid_body { create_rigidbody(rb_cinfo, use_transform_size) },
			is_kinematic {false}
		{
			rigid_body->setUserPointer(&parent); // sets user pointer used when resolving collisions
		}

		RigidBodyComponent(Entity& parent, PhysicsEngine& phy_engine, RigidBodyCreateInfo rb_cinfo, CollisionFilter cf, bool use_transform_size = false) :
			Component(parent),
			physics_engine{&phy_engine},
			rigid_body { create_rigidbody(rb_cinfo, cf, use_transform_size) },
			is_kinematic {false}
		{
			rigid_body->setUserPointer(&parent); // sets user pointer used when resolving collisions
		}

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
				glm::vec3 size = parent->transform().size();

				// we take the transformation matrix of the rigid boby, as calculated by the physics engine
				rigid_body->getMotionState()->getWorldTransform(bt_transform);

				// we convert the Bullet matrix (transform) to an array of floats
				bt_transform.getOpenGLMatrix(matrix);

				// Bullet matrix provides rotations and translations: it does not consider scale
				parent->set_transform(glm::make_mat4(matrix) * glm::scale(glm::mat4{ 1.0f }, size), false);
			}
		}

		int type()
		{
			return COMPONENT_ID;
		}

		// Syncs physics position with parent entity transform
		void on_transform_update()
		{
			reset_transform(parent->transform());
		}

	private:
		btRigidBody* create_rigidbody(RigidBodyCreateInfo rb_cinfo, bool use_transform_size = false)
		{
			if (use_transform_size) rb_cinfo.cs_info.size = parent->transform().size();

			return physics_engine->addRigidBody(parent->transform().position(), parent->transform().orientation(), rb_cinfo);
		}

		btRigidBody* create_rigidbody(RigidBodyCreateInfo rb_cinfo, CollisionFilter cf, bool use_transform_size = false)
		{
			if (use_transform_size) rb_cinfo.cs_info.size = parent->transform().size();

			return physics_engine->addRigidBody(parent->transform().position(), parent->transform().orientation(), rb_cinfo, cf);
		}

		void reset_forces()
		{
			// Resetting forces and velocities
			rigid_body->setLinearVelocity(btVector3{ 0,0,0 });
			rigid_body->setAngularVelocity(btVector3{ 0,0,0 });
			rigid_body->clearForces();

			rigid_body->activate();
		}

		void reset_bt_transform(const btTransform& new_transform)
		{
			// Updating physics transforms
			rigid_body->setWorldTransform(new_transform);
			rigid_body->getMotionState()->setWorldTransform(new_transform);
		}

		void reset_transform(const Transform& new_transform)
		{
			// Copying old transform and changing the position
			btTransform old_physics_transform = rigid_body->getWorldTransform();

			btVector3 new_pos{ new_transform.position().x, new_transform.position().y, new_transform.position().z };
			old_physics_transform.setOrigin(new_pos);

			glm::vec3 rot_radians {glm::radians(new_transform.orientation())};
			btQuaternion new_rot;
			new_rot.setEuler(rot_radians.y, rot_radians.x, rot_radians.z);
			
			old_physics_transform.setRotation(new_rot);

			reset_bt_transform(old_physics_transform);
			reset_forces();
		}
		
		void reset_position(const glm::vec3& new_position)
		{
			// Copying old transform and changing the position
			btTransform old_physics_transform = rigid_body->getWorldTransform();
			btVector3 new_pos{ new_position.x, new_position.y, new_position.z };
			old_physics_transform.setOrigin(new_pos);

			reset_bt_transform(old_physics_transform);
			reset_forces();
		}
	};
}