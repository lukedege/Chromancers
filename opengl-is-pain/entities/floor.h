#pragma once


#include "../utils/model.h"
#include "../utils/material.h"
#include "../utils/scene/entity.h"
#include "../utils/scene/scene.h"

using namespace utils::graphics::opengl;


class Floor : public Entity
{
public:
	Floor(Model& drawable, Material& material, SceneData& current_scene, float norm_map_repeat = 80.f) :
		Entity{ drawable, material, current_scene }
	{

	}
	
	void prepare_draw() const noexcept
	{
	}

	void child_update(float delta_time) noexcept
	{
		
	}
};