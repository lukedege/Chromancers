#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include <limits>

#include "camera.h"
#include "light.h"
#include "entity.h"

namespace engine::scene
{
	class Scene
	{
	private:
		std::unordered_map<std::string, std::unique_ptr<Entity>> entities; // unique pointers to elements in the vector will be valid even after the vector is resized    
		std::vector<std::string> marked_for_removal; 

		// For random ids
		std::random_device random_device; // create object for seeding
		std::mt19937 engine{random_device()}; // create engine and seed it
		std::uniform_int_distribution<unsigned int> distribution{0, std::numeric_limits<unsigned int>::max()}; // create distribution for integers with given range

	public:
		Camera* current_camera;

		template <typename ...Args>
		Entity* emplace_entity(std::string key, Args&&... args)
		{
			std::string final_key = key;
			while (entities.contains(final_key))
			{
				unsigned int random_number = distribution(engine);
				final_key = key + std::to_string(random_number);
				//utils::io::warn("SCENE - The scene already contains an entity with id ", key, ". For disambiguation purposes, its new id will be ", final_key);
			}
			auto add_result = entities.emplace(final_key, std::make_unique<Entity>( args... ));

			// (pair <iterator, bool>) -> (pair <key, value>) -> value -> field
			add_result.first->second->_scene_state.current_scene = this; // setting this scene as current
			add_result.first->second->_scene_state.entity_id = final_key; // setting the id in this scene

			return add_result.first->second.get(); // return the raw pointer
		}

		void mark_for_removal(const std::string& id_to_remove);

		void remove_marked();

		void update(float deltaTime);

		void draw() const;

		void custom_draw(Shader& shader) const;
	};
}