#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <utility>

#include "../random.h"
#include "../shader.h"
#include "../material.h"
#include "../utils.h"

#include "camera.h"
#include "entity.h"

namespace engine::scene
{
	namespace details
	{
		struct string_pair_hash 
		{
			inline std::size_t operator()(const std::pair<std::string, std::string> & v) const 
			{
				std::hash<std::string> hasher;
				return hasher(v.first+v.second);
			}
		};
	}

	// Class that represents a collection of entities 
	class Scene
	{
	private:
		using Shader = engine::resources::Shader;
		using Material = engine::resources::Material;

		using entity_map = std::unordered_map<std::string, std::shared_ptr<Entity>>; // unique pointers to elements in the vector will be valid even after the vector is resized 
		using entity_group_map = std::unordered_map<std::string, entity_map>; // we assume that each entity in a group uses the same material

		// Entities which will be drawn indipendently with their own material
		entity_map entities; 

		// Groups of entities which will be drawn together in instanced mode, sharing a material (thus a shader)
		entity_group_map instanced_entities_groups; 
		
		// Collection of ids of marked entities to destroy at the end of the loop
		std::unordered_set<std::string> marked_for_removal; 

		// Collection of instanced entity ids to find and destroy at the end of the loop
		std::unordered_set<std::pair<std::string, std::string>, details::string_pair_hash> marked_for_removal_instanced; 

		GLuint instanced_ssbo{ 0 }; // ID of an OpenGL SSBO (Shader Storage Buffer Object) containing instanced entities' transforms
		std::vector<glm::mat4> instance_group_transforms; // Collection of instanced entities' transforms

		// Draws the provided entities (using the custom shader if given)
		void draw_internal(entity_map entities, entity_group_map instanced_entities_groups, Shader* custom_shader = nullptr);

	public:
		Camera* current_camera{ nullptr };
		utils::random::generator& rng;

		Scene(utils::random::generator& rng) : rng{rng} { glGenBuffers(1, &instanced_ssbo); }

		// Emplaces a entity into the indepented entities collection given its construction arguments and returns a raw ptr to it
		template <typename ...Args>
		Entity* emplace_entity(std::string entity_id, Args&&... args)
		{
			std::string final_id = try_get_unique_string_id(entities, entity_id);
			auto add_result = entities.emplace(final_id, std::make_shared<Entity>( args... ));
			Entity* newly_added_entity = add_result.first->second.get(); // emplace returns (pair <iterator, bool>) -> (pair <key, value>) -> value -> field

			// Setting up entity's scene state
			newly_added_entity->_scene_state.current_scene = this; // setting this scene as current
			newly_added_entity->_scene_state.entity_id = final_id; // setting the id in this scene
			newly_added_entity->_scene_state.instanced_group_id = std::nullopt; // setting the group id to nullopt since we are drawing it independently

			return newly_added_entity; // return the raw pointer
		}

		// Emplaces a entity into the instnced entities collection given its construction arguments and returns a raw ptr to it
		template <typename... Args>
		Entity* emplace_instanced_entity(std::string group_id, std::string entity_id, Args&&... args)
		{
			std::string final_entity_id = try_get_unique_string_id(instanced_entities_groups[group_id], entity_id);
			auto add_result = instanced_entities_groups[group_id].emplace(final_entity_id, std::make_shared<Entity>(args...));
			Entity* newly_added_entity = add_result.first->second.get(); // emplace returns (pair <iterator, bool>) -> (pair <key, value>) -> value -> field
			
			// Setting up entity's scene state
			newly_added_entity->_scene_state.current_scene = this; // setting this scene as current
			newly_added_entity->_scene_state.entity_id = final_entity_id; // setting the id in this scene
			newly_added_entity->_scene_state.instanced_group_id = group_id; // setting the group id 
			
			return newly_added_entity; // return the raw pointer
		}

		// Marks an entity for removal given its id (and optionally its group_id if its an instanced entity)
		void mark_for_removal(const std::string& id_to_remove, std::optional<std::string> group_id = std::nullopt);

		// Deletes all marked entities
		void remove_marked();

		// Calls the init method for every entity
		void init();

		// Calls the update method for every entity
		void update(float deltaTime);

		// Draw all entities
		void draw(Shader* custom_shader = nullptr);

		// Draw only independent entities
		void draw_except_instanced(Shader* custom_shader = nullptr);

		// Draw only instanced entities
		void draw_only_instanced  (Shader* custom_shader = nullptr);

		// Draw only entities with the same id as the ones contained in the given collection
		void draw_only (const std::vector<std::string>& ids_to_draw, Shader* custom_shader = nullptr);

		// Draw all entities except the ones with the same id as the ones contained in the given collection
		void draw_except(const std::vector<std::string>& ids_to_not_draw, Shader* custom_shader = nullptr);

	private:

		// Function that returns an unique string id (not in the given map) given a base string
		template<typename T>
		std::string try_get_unique_string_id(const std::unordered_map<std::string, T>& map, const std::string& id)
		{
			std::string final_id = id;

			// If map contains the id, append a random uint to the base id string and check until it is unique
			while (map.contains(final_id))
			{
				unsigned int random_number = rng.get_uint();
				final_id = id + std::to_string(random_number);
				//utils::io::warn("SCENE - The scene already contains an entity with id ", key, ". For disambiguation purposes, its new id will be ", final_key);
			}
			return final_id;
		}
	};
}