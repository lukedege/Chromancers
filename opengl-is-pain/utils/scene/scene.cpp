#include "scene.h"

namespace engine::scene
{
	void Scene::mark_for_removal(const std::string& id_to_remove, std::optional<std::string> group_id)
	{
		if(group_id.has_value())
			marked_for_removal_instanced.emplace_back(group_id.value(), id_to_remove);
		else
			marked_for_removal.emplace_back(id_to_remove);
	}

	void Scene::remove_marked()
	{
		for (const std::string& entity_id : marked_for_removal)
		{
			entities.erase(entity_id);
		}
		for (const auto& [group_id, entity_id] : marked_for_removal_instanced)
		{
			instanced_entities_groups[group_id].erase(entity_id);
		}
		marked_for_removal.clear();
		marked_for_removal_instanced.clear();
	}

	void Scene::init()
	{
		for (auto& [id, entity] : entities)
		{
			entity->init();
		}
		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			for (auto& [id, instanced_entity] : instanced_group)
			{
				instanced_entity->init();
			}
		}
		
	}

	void Scene::update(float deltaTime)
	{
		for (auto& [id, entity] : entities)
		{
			entity->update(deltaTime);
		}

		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			for (auto& [id, instanced_entity] : instanced_group)
			{
				instanced_entity->update(deltaTime);
			}
		}
	}

	void Scene::draw() const
	{
		// Draw independent entities
		for (auto& [id, entity] : entities)
		{
			entity->draw();
		}

		// Draw instanced groups of entities
		Material* current_group_material;
		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			// get first entity material, we're assuming all entities in a group share the same material
			if (instanced_group.size() > 0)
			{
				current_group_material = instanced_group.begin()->second->material; 
				current_group_material->bind();
				for (auto& [id, instanced_entity] : instanced_group)
				{
					instanced_entity->custom_draw(*current_group_material->shader);
				}
				current_group_material->unbind();
			}
		}
	}

	void Scene::custom_draw(Shader& shader) const
	{
		for (auto& [id, entity] : entities)
		{
			entity->custom_draw(shader);
		}
	}
}