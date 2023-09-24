#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../model.h"
#include "../shader.h"
#include "../material.h"
#include "../scene/scene.h"
#include "../physics.h"
#include "../component.h"

#include "transform.h"

namespace utils::graphics::opengl
{
	// Object in scene
	class Entity
	{
	protected:
		Model* model; 
		SceneData* current_scene; // an entity can concurrently exist in only one scene at a time

	public:
		Transform transform;
		Material* material;
		std::vector<Component*> components; // map should be okay 

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

			// TODO check order of updates (components or child first?)
			for (Component* c : components)
			{
				c->update(delta_time);
			}
		}

		void set_scene(SceneData& scene_data)
		{
			current_scene = &scene_data;
		}

		void teleport_at(const glm::vec3 new_position)
		{
			transform.set_position(new_position);
			
			// We need to sync rigidbodies
			for (Component* c : components)
			{
				c->sync();
			}
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

	};
}