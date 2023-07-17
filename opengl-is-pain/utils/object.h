#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils/model.h"
#include "utils/shader.h"

namespace utils::graphics::opengl
{

	// Object in scene
	template <typename drawable_T>
	class Object
	{
		drawable_T drawable; // can be either Model or Mesh
		glm::mat4 transform;
		glm::mat3 normal;

	public:
		Object(drawable_T&  drawable, const glm::mat4& transform = glm::mat4(1)) : drawable{ std::move(drawable) }, transform{ transform }, normal{ glm::mat3(1) } {}
		Object(drawable_T&& drawable, const glm::mat4& transform = glm::mat4(1)) : drawable{ std::move(drawable) }, transform{ transform }, normal{ glm::mat3(1) } {}

		void scale(glm::vec3 scaling) { transform = glm::scale(transform, scaling); }
		void translate(glm::vec3 translation) { transform = glm::translate(transform, translation); }
		void rotate(float angle_rad, glm::vec3 rotationAxis) { transform = glm::rotate(transform, angle_rad, rotationAxis); }
		void rotate_deg(float angle_deg, glm::vec3 rotationAxis) { rotate(glm::radians(angle_deg), rotationAxis); }

		void draw(const Shader& shader, const glm::mat4& viewProjection)
		{
			shader.use();
			recomputeNormal(viewProjection);

			shader.setMat4("modelMatrix", transform);
			shader.setMat3("normalMatrix", normal);

			drawable.draw();
		}

		void draw(GLuint ubo_obj_matrices, const glm::mat4& viewProjection)
		{
			recomputeNormal(viewProjection);

			glBindBuffer(GL_UNIFORM_BUFFER, ubo_obj_matrices);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(transform));
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), glm::value_ptr(normal));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), 1, (void*)glm::value_ptr(transform));
			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), 1, (void*)glm::value_ptr(normal));

			drawable.draw();
		}

		void reset_transform()
		{
			// reset to identity
			transform = glm::mat4(1);
			normal = glm::mat3(1);
		}

	private:
		void recomputeNormal(glm::mat4 viewProjection) { normal = glm::inverseTranspose(glm::mat3(viewProjection * transform)); }
	};
}