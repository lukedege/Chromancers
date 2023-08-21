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

#include "transform.h"

namespace utils::graphics::opengl
{
	// Object in scene
	template <is_drawable drawable_T>
	class Entity
	{
		// TODO when physics done -> size_t bullet_id
		// mesh
		drawable_T drawable; // can be either Model or Mesh
		
	public:
		Entity(drawable_T&  drawable, Material& material) : drawable{ std::move(drawable) }, material{ &material } {}
		Entity(drawable_T&& drawable, Material& material) : drawable{ std::move(drawable) }, material{ &material } {}

		Transform transform;
		Material* material;

		void draw(const glm::mat4& view_matrix) const noexcept
		{
			material->use();

			material->shader->setMat4("viewMatrix", view_matrix);
			material->shader->setMat4("modelMatrix", transform.world_matrix());
			material->shader->setMat3("normalMatrix", compute_normal(view_matrix));

			drawable.draw();
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

	private:
		glm::mat3 compute_normal(glm::mat4 view_matrix) const noexcept { return glm::inverseTranspose(glm::mat3(view_matrix * transform.world_matrix())); }
	};
}