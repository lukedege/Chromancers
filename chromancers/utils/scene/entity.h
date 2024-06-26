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
#include "bounding_volume.h"

namespace engine::scene
{
	class Scene; // Forward declaration of Scene class (we aren't using it explicitly so we don't need the include, avoiding the cyclical include)

	// Class representing an object inside the game world 
	class EntityBase : utils::oop::non_movable
	{
	protected:
		friend class Scene;

		using Component = engine::components::Component;

		// This is a record for the entity to remember 
		// - in which scene it is into (current_scene)
		// - by which string id its identified in it (entity_id)
		// - (optional) in which group of instanced drawing elements it belongs to (instanced_group_id)
		struct SceneState
		{
			Scene* current_scene;
			std::string entity_id;
			std::optional<std::string> instanced_group_id;
		} _scene_state;

		Transform _local_transform; // local transform (relative to parent)
		Transform _world_transform; // world transform (got by calculating local_transform * parent world_transform)

		std::vector<std::unique_ptr<Component>> components; // Collection of components this entity owns

	public:
		EntityBase* parent{ nullptr }; // Parent entity (can be null)
		std::string display_name; // Display-friendly name for the entity

		EntityBase(std::string display_name = "");

		~EntityBase();

		// Calls the init method for every component
		void init() noexcept;

		// Calls the update method for every component and keep transforms updated
		void update(float delta_time) noexcept;

		// Callback for when the entity is involved in a collision
		void on_collision(Entity& other, glm::vec3 contact_point, glm::vec3 norm, glm::vec3 impulse);

		// Emplaces a component into the entity's collection given its construction arguments and returns a raw ptr to it
		// N.B. no need to pass the entity itself as argument from the outside
		template <typename ComponentType, typename ...Args>
		auto emplace_component(Args&&... args)
		{
			return components.emplace_back(std::make_unique<ComponentType>(*this, args...)).get();
		}

		// Finds and returns a raw ptr to the first component matching the type provided, nullptr otherwise
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
		const Transform& local_transform() const noexcept;
		const Transform& world_transform() const noexcept;

		void set_position  (const glm::vec3& new_position)    noexcept;
		void set_orientation  (const glm::vec3& new_orientation) noexcept;
		void set_size      (const glm::vec3& new_size)        noexcept;

		void translate     (const glm::vec3& translation)     noexcept;
		void rotate        (const glm::vec3& rotation)        noexcept;
		void scale         (const glm::vec3& scale)           noexcept;

		void set_transform (const glm::mat4& matrix, bool trigger_update = true) noexcept;
		
	protected:
		// Function to update the world transform after the local transform has changed
		// e.g. through set functions, from rigidbody syncing or from parent syncing
		void update_world_transform();

		// Callback for when the entity's transform has changed
		void on_transform_update();
#pragma endregion transform_stuff
	};

	// Class representing a visual object inside the game world 
	class Entity : public EntityBase
	{
		friend class Scene; // friendship isn't inheritable so we have to respecify it

		using Shader = engine::resources::Shader;
		using Model = engine::resources::Model;
		using Material = engine::resources::Material;

	public:
		Model* model;       // Pointer to the model representing the entity
		Material* material; // Pointer to the material representing the entity

		std::unique_ptr<BoundingVolume> bounding_volume; 

		Entity(std::string display_name, Model& drawable, Material& material);

		// Emplaces a component into the entity's collection given its construction arguments and returns a raw ptr to it
		// N.B. no need to pass the entity itself as argument from the outside
		template <typename ComponentType, typename ...Args>
		auto emplace_component(Args&&... args)
		{
			return components.emplace_back(std::make_unique<ComponentType>(*this, args...)).get();
		}

		// Draws the entity using the provided shader instead of the one included in the material
		void custom_draw(const Shader& shader) const noexcept;

		// Draws the entity using its material
		void draw() const noexcept;

	};
}