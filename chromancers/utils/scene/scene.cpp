#include "scene.h"

namespace engine::scene
{
	void Scene::mark_for_removal(const std::string& id_to_remove, std::optional<std::string> group_id)
	{
		if(group_id.has_value())
			marked_for_removal_instanced.emplace(group_id.value(), id_to_remove);
		else
			marked_for_removal.emplace(id_to_remove);
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
		glGenBuffers(1, &instanced_ssbo);

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

	void Scene::draw()
	{
		// Draw independent entities
		for (auto& [id, entity] : entities)
		{
			entity->draw();
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);

		// Draw instanced groups of entities
		Material* current_group_material;
		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			instance_group_transforms.clear();
			// get first entity material, we're assuming all entities in a group share the same material and model
			if (instanced_group.size() > 0)
			{
				current_group_material = instanced_group.begin()->second->material;
				current_group_material->bind();
				for (auto& [id, instanced_entity] : instanced_group)
				{
					// Fill data about instanced group transforms
					instance_group_transforms.push_back(instanced_entity->world_transform().matrix());

				}
				// Fill the shader's ubo/ssbo with the gathered transform data
				utils::graphics::opengl::setup_buffer_object(instanced_ssbo, GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4), instance_group_transforms.size(), glm::value_ptr(instance_group_transforms[0]));
				
				// Perform the instanced draw on the common model of the group
				instanced_group.begin()->second->model->draw_instanced(instance_group_transforms.size());

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