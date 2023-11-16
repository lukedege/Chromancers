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
		for (const std::string& entity_id : marked_for_removal)// TODO temp
		{
			instanced_entities.erase(entity_id);
		}
		marked_for_removal.clear();
	}

	void Scene::init()
	{
		for (auto& [id, entity] : entities)
		{
			entity->init();
		}
		for (auto& [id, entity] : instanced_entities)// TODO temp
		{
			entity->init();
		}
	}

	void Scene::update(float deltaTime)
	{
		for (auto& [id, entity] : entities)
		{
			entity->update(deltaTime);
		}

		for (auto& [id, entity] : instanced_entities)// TODO temp
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

	void Scene::instanced_draw(Material& m) const// TODO temp
	{
		m.bind();
		for (auto& [id, entity] : instanced_entities)
		{
			// TODO replace this with actual opengl instanced drawing
			entity->custom_draw(*m.shader);

		}
		m.unbind();
	}

	void Scene::custom_draw(Shader& shader) const
	{
		for (auto& [id, entity] : entities)
		{
			entity->custom_draw(shader);
		}
	}
}