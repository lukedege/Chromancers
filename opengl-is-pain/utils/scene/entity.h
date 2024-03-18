#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <optional>

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

namespace engine::scene
{
	class Scene; // Forward declaration of Scene class (we aren't using it explicitly so we don't need the include, avoiding the cyclical include)

	class EntityBase : utils::oop::non_movable
	{
	public:
		EntityBase(std::string display_name = "") :
			display_name{ display_name }
		{}

	protected:
		friend class Scene;

		// This is a record for the entity to remember 
		// - to which scene it is into (current_scene)
		// - by which string id its identified in it (entity_id)
		// - (optional) in which group of instanced drawing elements it belongs to (instanced_group_id)
		struct SceneState
		{
			Scene* current_scene;
			std::string entity_id;
			std::optional<std::string> instanced_group_id;
		};

		using Component = engine::components::Component;

		std::string display_name; 
		Transform _transform;
		SceneState _scene_state;
		std::vector<std::unique_ptr<Component>> components; // map should be okay also
	};

	// Object in scene
	class Entity : EntityBase
	{
		friend class Scene;

		using Shader = engine::resources::Shader;
		using Model = engine::resources::Model;
		using Material = engine::resources::Material;
		using Component = engine::components::Component;

	public:
		Material* material;
		Model* model; 

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
		virtual void specializated_update(float delta_time) noexcept;

		void on_transform_update();

	};
}