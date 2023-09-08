#pragma once


#include "../utils/model.h"
#include "../utils/material.h"
#include "../utils/scene/entity.h"
#include "../utils/scene/scene.h"

using namespace utils::graphics::opengl;


class Floor : public Entity
{
public:
	float norm_map_repeat;

	Floor(Model& drawable, Material& material, SceneData& current_scene, float norm_map_repeat = 80.f) :
		Entity{ drawable, material, current_scene },
		norm_map_repeat{ norm_map_repeat } 
	{}
	
	void prepare_draw() const noexcept
	{
		material->bind();

		material->shader->setFloat("repeat", norm_map_repeat);

		if (current_scene)
		{
			material->shader->setVec3("wCameraPos", current_scene->current_camera->position());
		}

		material->unbind();
	}

	void child_update(float delta_time) noexcept
	{
		
	}
};