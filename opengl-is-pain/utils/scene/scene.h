#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "../random.h"
#include "../shader.h"
#include "../material.h"

#include "camera.h"
#include "entity.h"

namespace engine::scene
{
	class Scene
	{
	private:
		using Shader = engine::resources::Shader;
		using Material = engine::resources::Material;

		std::unordered_map<std::string, std::unique_ptr<Entity>> entities; // unique pointers to elements in the vector will be valid even after the vector is resized    
		std::unordered_map<std::string, std::unique_ptr<Entity>> instanced_entities; // TODO assume for now that they use the same material
		std::vector<std::string> marked_for_removal; 

	public:
		Camera* current_camera;
		utils::random::generator rng;

		template <typename ...Args>
		Entity* emplace_entity(std::string key, Args&&... args)
		{
			std::string final_key = key;
			while (entities.contains(final_key))
			{
				unsigned int random_number = rng.get_uint();
				final_key = key + std::to_string(random_number);
				//utils::io::warn("SCENE - The scene already contains an entity with id ", key, ". For disambiguation purposes, its new id will be ", final_key);
			}
			auto add_result = entities.emplace(final_key, std::make_unique<Entity>( args... ));

			// (pair <iterator, bool>) -> (pair <key, value>) -> value -> field
			add_result.first->second->_scene_state.current_scene = this; // setting this scene as current
			add_result.first->second->_scene_state.entity_id = final_key; // setting the id in this scene

			return add_result.first->second.get(); // return the raw pointer
		}

		template <typename ...Args>
		Entity* emplace_instanced_entity(std::string key, Args&&... args)
		{
			std::string final_key = key;
			while (instanced_entities.contains(final_key))
			{
				unsigned int random_number = rng.get_uint();
				final_key = key + std::to_string(random_number);
				//utils::io::warn("SCENE - The scene already contains an entity with id ", key, ". For disambiguation purposes, its new id will be ", final_key);
			}
			auto add_result = instanced_entities.emplace(final_key, std::make_unique<Entity>( args... ));

			// (pair <iterator, bool>) -> (pair <key, value>) -> value -> field
			add_result.first->second->_scene_state.current_scene = this; // setting this scene as current
			add_result.first->second->_scene_state.entity_id = final_key; // setting the id in this scene

			return add_result.first->second.get(); // return the raw pointer
		}

		void mark_for_removal(const std::string& id_to_remove);

		void remove_marked();

		void init();

		void update(float deltaTime);

		void draw() const;

		void instanced_draw(Material& m) const;

		void custom_draw(Shader& shader) const;
	};
}