#pragma once

#include "../component.h"
#include "../physics.h"
#include "../transform.h"

namespace engine::components
{
	// Component that gives an entity a physical rigidbody and keeps its transform synced with its counterpart in the physics world
	class RigidBodyComponent : public Component
	{
		using PhysicsEngine = engine::physics::PhysicsEngine<Entity>;
		using RigidBodyCreateInfo = engine::physics::RigidBodyCreateInfo;
		using CollisionFilter = engine::physics::CollisionFilter;

	public:
		constexpr static auto COMPONENT_ID = 0;
	private:
		PhysicsEngine* physics_engine; // Pointer to the physics engine
		bool is_kinematic; // If body mass is zero, we don't need to update it through physics

	public:
		btRigidBody* rigid_body; // Pointer to the rigidbody

		RigidBodyComponent(Entity& parent, PhysicsEngine& phy_engine, RigidBodyCreateInfo rb_cinfo, bool use_transform_size = false) :
			Component(parent),
			physics_engine{&phy_engine},
			rigid_body { create_rigidbody(rb_cinfo, use_transform_size) },
			is_kinematic {rb_cinfo.mass <= 0}
		{
			rigid_body->setUserPointer(&parent); // sets parent entity as user pointer used when resolving collisions
		}

		RigidBodyComponent(Entity& parent, PhysicsEngine& phy_engine, RigidBodyCreateInfo rb_cinfo, CollisionFilter cf, bool use_transform_size = false) :
			Component(parent),
			physics_engine{&phy_engine},
			rigid_body { create_rigidbody(rb_cinfo, cf, use_transform_size) },
			is_kinematic {rb_cinfo.mass <= 0}
		{
			rigid_body->setUserPointer(&parent); // sets parent entity as user pointer used when resolving collisions
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
				glm::vec3 size = _parent->world_transform().size();

				// we take the transformation matrix of the rigid body, as calculated by the physics engine
				rigid_body->getMotionState()->getWorldTransform(bt_transform);

				// we convert the Bullet matrix (transform) to an array of floats
				bt_transform.getOpenGLMatrix(matrix);
				
				// Bullet matrix provides rotations and translations: it does not consider scale
				glm::mat4 updated_transform = glm::make_mat4(matrix) * glm::scale(glm::mat4{ 1.0f }, size);

				// Set the parent entity transform without triggering "on_transform_update()" events to avoid infinite recursion
				_parent->set_transform(updated_transform, false);
			}
		}

		int type()
		{
			return COMPONENT_ID;
		}

		void on_transform_update()
		{
			// Syncs physics position with parent entity transform
			reset_transform(_parent->world_transform());
		}

	private:
		// Creates and add a rigidbody to the dynamic world through the physics engine
		btRigidBody* create_rigidbody(RigidBodyCreateInfo rb_cinfo, bool use_transform_size = false)
		{
			if (use_transform_size) rb_cinfo.cs_info.size = _parent->local_transform().size();

			return physics_engine->addRigidBody(_parent->world_transform().position(), _parent->world_transform().orientation(), rb_cinfo);
		}

		// Creates and add a rigidbody to the dynamic world providing a collision filter for it through the physics engine
		btRigidBody* create_rigidbody(RigidBodyCreateInfo rb_cinfo, CollisionFilter cf, bool use_transform_size = false)
		{
			if (use_transform_size) rb_cinfo.cs_info.size = _parent->world_transform().size();

			return physics_engine->addRigidBody(_parent->world_transform().position(), _parent->world_transform().orientation(), rb_cinfo, cf);
		}

		// Resets forces and velocities on the rigidbody
		void reset_forces()
		{
			rigid_body->setLinearVelocity(btVector3{ 0,0,0 });
			rigid_body->setAngularVelocity(btVector3{ 0,0,0 });
			rigid_body->clearForces();

			rigid_body->activate();
		}

		// Resets rigidbody's internal transform given a Bullet transform
		void reset_bt_transform(const btTransform& new_transform)
		{
			// Updating physics transforms
			rigid_body->setWorldTransform(new_transform);
			rigid_body->getMotionState()->setWorldTransform(new_transform);
		}

		// Resets rigidbody's internal transform given an entity transform
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
		
		// Resets rigidbody's internal position given a glm vec3
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