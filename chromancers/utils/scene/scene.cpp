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

	// Draw the complete scene
	void Scene::draw(Shader* custom_shader)
	{
		draw_internal(entities, instanced_entities_groups, custom_shader);
	}

	void Scene::draw_except_instanced(Shader* custom_shader)
	{
		for (auto& [id, entity] : entities)
		{
			if (custom_shader)
				entity->custom_draw(*custom_shader);
			else
				entity->draw();
		}
	}

	void Scene::draw_only_instanced(Shader* custom_shader)
	{
		// Lambda for drawing instanced groups of entities
		auto draw_group = [&](auto instanced_group)
		{
			for (auto& [id, instanced_entity] : instanced_group)
			{
				// Fill data about instanced group transforms
				instance_group_transforms.push_back(instanced_entity->world_transform().matrix());

			}
			// Fill the shader's ubo/ssbo with the gathered transform data
			utils::graphics::opengl::setup_buffer_object(instanced_ssbo, GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4), instance_group_transforms.size(), glm::value_ptr(instance_group_transforms[0]));

			// Perform the instanced draw on the common model of the group
			instanced_group.begin()->second->model->draw_instanced(instance_group_transforms.size());
		};

		Material* current_group_material;
		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			instance_group_transforms.clear();
			// get first entity material, we're assuming all entities in a group share the same material and model
			if (instanced_group.size() > 0)
			{
				if (custom_shader)
				{
					custom_shader->bind();
					draw_group(instanced_group);
					custom_shader->unbind();
				}
				else
				{
					current_group_material = instanced_group.begin()->second->material;
					current_group_material->bind();
					draw_group(instanced_group);
					current_group_material->unbind();
				}
				
			}
		}
	}

	// Draw only if the id is in the provided vector
	void Scene::draw_only(const std::vector<std::string>& ids_to_draw, Shader* custom_shader)
	{
		entity_map draw_entities;
		entity_group_map draw_groups;

		for (std::string id_to_draw : ids_to_draw)
		{
			auto it = entities.find(id_to_draw);
			if(it != entities.end())
			{
				draw_entities[id_to_draw] = it->second;
			}

			auto it_g = instanced_entities_groups.find(id_to_draw);
			if(it_g != instanced_entities_groups.end())
			{
				draw_groups[id_to_draw] = it_g->second;
			}
		}

		draw_internal(draw_entities, draw_groups, custom_shader);
	}

	void Scene::draw_except(const std::vector<std::string>& ids_to_not_draw, Shader* custom_shader)
	{
		entity_map draw_entities = entities;
		entity_group_map draw_groups = instanced_entities_groups;

		for (std::string id_to_draw : ids_to_not_draw)
		{
			draw_entities.erase(id_to_draw);
			draw_groups.erase(id_to_draw);
		}

		draw_internal(draw_entities, draw_groups, custom_shader);
	}

	void Scene::draw_internal(entity_map entities, entity_group_map instanced_entities_groups, Shader* custom_shader)
	{
		// Draw independent entities
		for (auto& [id, entity] : entities)
		{
			if (custom_shader)
				entity->custom_draw(*custom_shader);
			else
				entity->draw();
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);

		// Lambda for drawing instanced groups of entities
		auto draw_group = [&](auto instanced_group)
		{
			for (auto& [id, instanced_entity] : instanced_group)
			{
				// Fill data about instanced group transforms
				instance_group_transforms.push_back(instanced_entity->world_transform().matrix());

			}
			// Fill the shader's ubo/ssbo with the gathered transform data
			utils::graphics::opengl::setup_buffer_object(instanced_ssbo, GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4), instance_group_transforms.size(), glm::value_ptr(instance_group_transforms[0]));

			// Perform the instanced draw on the common model of the group
			instanced_group.begin()->second->model->draw_instanced(instance_group_transforms.size());
		};

		Material* current_group_material;
		for (auto& [group_id, instanced_group] : instanced_entities_groups)
		{
			instance_group_transforms.clear();
			// get first entity material, we're assuming all entities in a group share the same material and model
			if (instanced_group.size() > 0)
			{
				if (custom_shader)
				{
					custom_shader->bind();
					draw_group(instanced_group);
					custom_shader->unbind();
				}
				else
				{
					current_group_material = instanced_group.begin()->second->material;
					current_group_material->bind();
					draw_group(instanced_group);
					current_group_material->unbind();
				}
				
			}
		}
	}
}