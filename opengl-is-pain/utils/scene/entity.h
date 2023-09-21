#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../interfaces.h"
#include "../model.h"
#include "../shader.h"
#include "../material.h"
#include "../scene/scene.h"
#include "../physics.h"

#include "transform.h"

namespace utils::graphics::opengl
{
	// Object in scene
	class Entity
	{
	protected:
		Model* model; 
		SceneData* current_scene; // an entity can concurrently exist in only one scene at a time
		bool is_kinematic;

	public:
		Transform transform;
		Material* material;
		btRigidBody* rigid_body;

		Entity(Model& drawable, Material& material, SceneData& scene) :
			model{ &drawable }, material{ &material }, current_scene{ &scene }
		{}

		void draw() const noexcept
		{
			prepare_draw();

			material->bind();
			
			glm::mat4 view_matrix = current_scene->current_camera->viewMatrix();
			material->shader->setVec3("wCameraPos", current_scene->current_camera->position());
			material->shader->setMat4("viewMatrix", view_matrix);
			material->shader->setMat4("modelMatrix", transform.world_matrix());
			material->shader->setMat3("normalMatrix", compute_normal(view_matrix));

			model->draw();
			material->unbind();
		}

		void update(float delta_time) noexcept 
		{
			child_update(delta_time);

			if(rigid_body && !is_kinematic)
				update_physics(delta_time);
		}

		void set_scene(SceneData& scene_data)
		{
			current_scene = &scene_data;
		}

		void teleport_at(const glm::vec3 new_position)
		{
			transform.set_position(new_position);

			if (rigid_body)
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
		}

		// Physics related
		void add_rigidbody(physics::PhysicsEngine::RigidBodyCreateInfo rb_cinfo)
		{
			rigid_body = current_scene->physics_engine->createRigidBody(transform.position(), transform.size(), transform.orientation(), rb_cinfo);
		}

		void remove_rigidbody()
		{
			current_scene->physics_engine->deleteRigidBody(rigid_body);
			rigid_body = nullptr;
		}

		void set_kinematic(bool is_kinematic)
		{
			is_kinematic = is_kinematic;
			// TODO check if rigid body's mass needs to be set to zero or this is enough
		}

		//void draw(GLuint ubo_obj_matrices, const glm::mat4& view_matrix)
		//{
		//	glBindBuffer(GL_UNIFORM_BUFFER, ubo_obj_matrices);
		//	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(transform.world_matrix()));
		//	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), glm::value_ptr(compute_normal(view_matrix)));
		//	glBindBuffer(GL_UNIFORM_BUFFER, 0);
		//
		//	//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), 1, (void*)glm::value_ptr(transform));
		//	//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), 1, (void*)glm::value_ptr(normal));
		//
		//	drawable.draw();
		//}

	protected:
		virtual void prepare_draw() const noexcept {}
		virtual void child_update(float delta_time) noexcept {}

	private:
		glm::mat3 compute_normal(glm::mat4 view_matrix) const noexcept { return glm::inverseTranspose(glm::mat3(view_matrix * transform.world_matrix())); }

		void update_physics(float delta_time)
		{
			GLfloat matrix[16];
			btTransform bt_transform;
			glm::vec3 size = transform.size();

			// we take the transformation matrix of the rigid boby, as calculated by the physics engine
			rigid_body->getMotionState()->getWorldTransform(bt_transform);

			// we convert the Bullet matrix (transform) to an array of floats
			bt_transform.getOpenGLMatrix(matrix);

			// Bullet matrix provides rotations and translations: it does not consider scale
			transform.set(glm::make_mat4(matrix) * glm::scale(glm::mat4{ 1.0f }, size));
		}
	};
}