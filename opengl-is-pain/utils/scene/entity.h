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

#include "transform.h"

namespace utils::graphics::opengl
{
	

	// Object in scene
	template <typename drawable_T>
	requires is_drawable<drawable_T>
	class Entity
	{
		drawable_T drawable; // can be either Model or Mesh
		glm::mat3 normal;
		std::unordered_map<std::string, std::unique_ptr<Component>> components;
		
	public:
		Entity(drawable_T&  drawable) : drawable{ std::move(drawable) }, normal{ glm::mat3(1) } {}
		Entity(drawable_T&& drawable) : drawable{ std::move(drawable) }, normal{ glm::mat3(1) } {}

		Transform transform;

		void draw(const Shader& shader, const glm::mat4& viewProjection)
		{
			shader.use();
			recompute_normal(viewProjection);

			shader.setMat4("modelMatrix", transform.world_matrix());
			shader.setMat3("normalMatrix", normal);

			drawable.draw();
		}

		void draw(GLuint ubo_obj_matrices, const glm::mat4& viewProjection)
		{
			recompute_normal(viewProjection);

			glBindBuffer(GL_UNIFORM_BUFFER, ubo_obj_matrices);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(transform.world_matrix()));
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), glm::value_ptr(normal));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), 1, (void*)glm::value_ptr(transform));
			//opengl::update_buffer_object(ubo_obj_matrices, GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat3), 1, (void*)glm::value_ptr(normal));

			drawable.draw();
		}

		template <typename Component_t, typename... Args>
		void add_component(const std::string key, Args&&... args)
		{
			components.emplace(key, std::make_unique<Component_t>(std::forward<Args>(args)...));
		}

		void remove_component(const std::string key)
		{
			components.erase(key);
		}

	private:
		void recompute_normal(glm::mat4 viewProjection) { normal = glm::inverseTranspose(glm::mat3(viewProjection * transform.world_matrix())); }
	};
}