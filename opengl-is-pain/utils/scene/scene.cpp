#include "scene.h"

namespace engine::scene
{
	void Scene::add_entity(Entity& entity)
	{
		entity.current_scene = this;
		auto add_result = entities.emplace(entity.name, &entity);
		if (!add_result.second)
			utils::io::warn("SCENE - The scene already contains an entity with that name");
		// TODO handle better when trying to add an entity with the same name (add a -001 at the end maybe?)
	}

	void Scene::mark_for_removal(std::string entity_name)
	{
		marked_for_removal.push_back(entity_name);
	}

	void Scene::remove_marked()
	{
		for (const std::string& entity_name : marked_for_removal)
		{
			entities.erase(entity_name);
		}
		marked_for_removal.clear();
	}

	void Scene::update(float deltaTime)
	{
		for (auto& [name, entity] : entities)
		{
			entity->update(deltaTime);
		}
	}

	void Scene::draw() const
	{
		for (auto& [name, entity] : entities)
		{
			entity->draw();
		}
	}

	void Scene::custom_draw(Shader& shader) const
	{
		for (auto& [name, entity] : entities)
		{
			entity->custom_draw(shader);
		}
	}
}