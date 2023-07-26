#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "transform.h"
#include "model.h"
#include "shader.h"

namespace utils::graphics::opengl
{

	// Object in scene
	template <typename drawable_T>
	class Entity
	{
		drawable_T drawable; // can be either Model or Mesh
		glm::mat3 normal;
		
	public:
		Entity(drawable_T&  drawable) : drawable{ std::move(drawable) }, normal{ glm::mat3(1) } {}
		Entity(drawable_T&& drawable) : drawable{ std::move(drawable) }, normal{ glm::mat3(1) } {}

		//void rotate(float angle_rad, glm::vec3 rotationAxis) { transform = glm::rotate(transform, angle_rad, rotationAxis); }
		//void rotate_deg(float angle_deg, glm::vec3 rotationAxis) { rotate(glm::radians(angle_deg), rotationAxis); }
		Transform transform;

		void draw(const Shader& shader, const glm::mat4& viewProjection)
		{
			shader.use();
			recomputeNormal(viewProjection);

			shader.setMat4("modelMatrix", transform.world_matrix());
			shader.setMat3("normalMatrix", normal);

			drawable.draw();
		}

		void draw(GLuint ubo_obj_matrices, const glm::mat4& viewProjection)
		{
			recomputeNormal(viewProjection);

			glBindBuffer(GL_UNIFORM_BUFFER, ubo_obj_matrices);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(transform.world_matrix()));
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), glm::value_ptr(normal));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), 1, (void*)glm::value_ptr(transform));
			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), 1, (void*)glm::value_ptr(normal));

			drawable.draw();
		}

		void reset_transform()
		{
			// reset to identity
			//transform = glm::mat4(1);
			normal = glm::mat3(1);
		}

	private:
		void recomputeNormal(glm::mat4 viewProjection) { normal = glm::inverseTranspose(glm::mat3(viewProjection * transform.world_matrix())); }
	};
}