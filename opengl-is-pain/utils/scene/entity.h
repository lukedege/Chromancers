#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "../oop.h"
#include "../io.h"
#include "../model.h"
#include "../shader.h"
#include "../material.h"
#include "../component.h"

#include "../transform.h"

#include "scene.h"

namespace
{
	using namespace engine::components;
}

namespace engine::scene
{
	class Scene; 
	struct SceneState
	{
		Scene* current_scene;
		std::string entity_id;
	};

	// Object in scene
	class Entity : utils::oop::non_movable
	{
		friend class Scene;
	protected:
		Transform _transform;
		Model* model; 
		SceneState _scene_state;
		std::vector<std::unique_ptr<Component>> components; // map should be okay also
	public:
		std::string display_name; 
		Material* material;

		Entity(std::string display_name, Model& drawable, Material& material);

		~Entity();

		// draws using the provided shader instead of the material
		void custom_draw(Shader& shader) const noexcept;

		void init() noexcept;

		void draw() const noexcept;

		void update(float delta_time) noexcept;

		void on_collision(Entity& other, glm::vec3 contact_point, glm::vec3 norm, glm::vec3 impulse);

		// N.B. no need to pass the entity as arg
		template <typename ComponentType, typename ...Args>
		void emplace_component(Args&&... args)
		{
			components.emplace_back(std::make_unique<ComponentType>(*this, args...));
		}

		// Returns a raw ptr to the first component matching the type provided, nullptr otherwise
		template <typename ComponentType>
		ComponentType* get_component()
		{
			ComponentType* to_find = nullptr;
			for (auto& c : components)
			{
				if (c->type() == ComponentType::COMPONENT_ID)
				{
					to_find = static_cast<ComponentType*>(c.get());
					break;
				}
			}
			return to_find;
		}

		const SceneState& scene_state() const;

#pragma region transform_stuff
		const Transform& transform() const noexcept;

		void set_position  (const glm::vec3& new_position)    noexcept;
		void set_rotation  (const glm::vec3& new_orientation) noexcept;
		void set_size      (const glm::vec3& new_size)        noexcept;

		void translate     (const glm::vec3& translation)     noexcept;
		void rotate        (const glm::vec3& rotation)        noexcept;
		void scale         (const glm::vec3& scale)           noexcept;

		void set_transform (const glm::mat4& matrix, bool trigger_update = true) noexcept;
#pragma endregion transform_stuff

	protected:
		virtual void prepare_draw() const noexcept;
		virtual void child_update(float delta_time) noexcept;

		void on_transform_update();

	};
}