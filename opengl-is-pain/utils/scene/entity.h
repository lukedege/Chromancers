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
#include "../component.h"

#include "../transform.h"

namespace
{
	using namespace engine::components;
}

namespace engine::scene
{
	// Object in scene
	class Entity
	{
	protected:
		Transform _transform;
		Model* model; 
		SceneData* current_scene; // an entity can concurrently exist in only one scene at a time
		std::string name;
	public:
		
		Material* material;
		std::vector<Component*> components; // map should be okay 

		Entity(std::string name, Model& drawable, Material& material, SceneData& scene) :
			name { name }, model{ &drawable }, material{ &material }, current_scene{ &scene }
		{}

		// only draws the mesh without considering the material
		void plain_draw() const noexcept
		{
			model->draw();
		}

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

		void on_collision(Entity& other, glm::vec3 contact_point)
		{
			//utils::io::log(name, " is colliding with ", other.name, " at coords ( ", contact_point.x, ", ", contact_point.y, ", ", contact_point.z, " )");
			for (Component* c : components)
			{
				c->on_collision(other, contact_point);
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

	};
}