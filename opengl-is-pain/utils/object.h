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
	class Object
	{
		Model model;
		glm::mat4 transform;
		glm::mat3 normal;

	public:
		Object(const std::string& modelPath, const glm::mat4& transform = glm::mat4(1)) : model{ modelPath }, transform{ transform }, normal{ glm::mat3(1) } {}
		Object(Model&& otherModel, const glm::mat4& transform = glm::mat4(1)) : model{ std::move(otherModel) }, transform{ transform }, normal{ glm::mat3(1) } {}

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

			model.draw();

			// reset to identity
			transform = glm::mat4(1);
			normal = glm::mat3(1);
		}
	private:
		void recomputeNormal(glm::mat4 viewProjection) { normal = glm::inverseTranspose(glm::mat3(viewProjection * transform)); }
	};
}