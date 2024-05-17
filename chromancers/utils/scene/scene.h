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
	class Scene
	{
	private:
		using Shader = engine::resources::Shader;
		using Material = engine::resources::Material;

		using entity_map = std::unordered_map<std::string, std::shared_ptr<Entity>>; // unique pointers to elements in the vector will be valid even after the vector is resized 
		using entity_group_map = std::unordered_map<std::string, entity_map>; // assume for now that they use the same material

		// Entities which will be drawn indipendently with their own material
		entity_map entities; 

		// Groups of entities which will be drawn together in instanced mode, sharing a material (thus a shader)
		entity_group_map instanced_entities_groups; 
		
		std::unordered_set<std::string> marked_for_removal; 
		std::unordered_set<std::pair<std::string, std::string>, details::string_pair_hash> marked_for_removal_instanced; 

		GLuint instanced_ssbo{ 0 }; // or ssbo
		std::vector<glm::mat4> instance_group_transforms;

		void draw_internal(entity_map entities, entity_group_map instanced_entities_groups, Shader* custom_shader = nullptr);

	public:
		Camera* current_camera{ nullptr };
		utils::random::generator& rng;

		Scene(utils::random::generator& rng) : rng{rng} {}

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

		void mark_for_removal(const std::string& id_to_remove, std::optional<std::string> group_id = std::nullopt);

		void remove_marked();

		void init();

		void update(float deltaTime);

		void draw(Shader* custom_shader = nullptr);

		void draw_except_instanced(Shader* custom_shader = nullptr);

		void draw_only_instanced  (Shader* custom_shader = nullptr);

		void draw_only (const std::vector<std::string>& ids_to_draw, Shader* custom_shader = nullptr);

		void draw_except(const std::vector<std::string>& ids_to_not_draw, Shader* custom_shader = nullptr);

	private:
		template<typename T>
		std::string try_get_unique_string_id(const std::unordered_map<std::string, T>& map, const std::string& id)
		{
			std::string final_id = id;
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