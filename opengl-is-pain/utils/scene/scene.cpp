#include "scene.h"

namespace engine::scene
{
	void Scene::mark_for_removal(const std::string& id_to_remove)
	{
		marked_for_removal.push_back(id_to_remove);
	}

	void Scene::remove_marked()
	{
		for (const std::string& entity_id : marked_for_removal)
		{
			entities.erase(entity_id);
		}
		marked_for_removal.clear();
	}

	void Scene::update(float deltaTime)
	{
		for (auto& [id, entity] : entities)
		{
			entity->update(deltaTime);
		}
	}

	void Scene::draw() const
	{
		for (auto& [id, entity] : entities)
		{
			entity->draw();
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