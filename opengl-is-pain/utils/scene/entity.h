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
		Transform _transform;
		Model* model; 
		SceneData* current_scene; // an entity can concurrently exist in only one scene at a time
	
	public:
		
		Material* material;
		std::vector<Component*> components; // map should be okay 

		Entity(Model& drawable, Material& material, SceneData& scene) :
			model{ &drawable }, material{ &material }, current_scene{ &scene }
		{}

		void draw() const noexcept
		{
			prepare_draw();

			material->bind();
			
			material->shader->setMat4("modelMatrix", _transform.world_matrix());

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

		void set_scene(SceneData& scene_data) noexcept
		{
			current_scene = &scene_data;
		}

#pragma region transform_stuff
		const Transform& transform() const noexcept { return _transform; }

		void set_position  (const glm::vec3& new_position)    noexcept { _transform.set_position(new_position);    on_transform_update(); }
		void set_rotation  (const glm::vec3& new_orientation) noexcept { _transform.set_rotation(new_orientation); on_transform_update(); }
		void set_size      (const glm::vec3& new_size)        noexcept { _transform.set_size(new_size);            on_transform_update(); }

		void translate     (const glm::vec3& translation)     noexcept { _transform.translate(translation); on_transform_update(); }
		void rotate        (const glm::vec3& rotation)        noexcept { _transform.rotate(rotation);       on_transform_update(); }
		void scale         (const glm::vec3& scale)           noexcept { _transform.scale(scale);           on_transform_update(); }

		void set_transform (const glm::mat4& matrix, bool trigger_update = true) noexcept { _transform.set(matrix); if(trigger_update) on_transform_update(); }
#pragma endregion transform_stuff

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

		void on_transform_update()
		{
			for (Component* c : components)
			{
				c->on_transform_update();
			}
		}

	private:

	};
}