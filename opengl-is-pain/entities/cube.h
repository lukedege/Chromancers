#pragma once

#include "../utils/model.h"
#include "../utils/material.h"
#include "../utils/scene/entity.h"
#include "../utils/scene/scene.h"

namespace
{
	using namespace engine::resources;
	using namespace engine::scene;
}
class Cube : public Entity
{
public:
	float spin_speed = 30.f;
	bool spinning = true;

	Cube(std::string name, Model& drawable, Material& material, SceneData& current_scene) :
		Entity{ name, drawable, material, current_scene }
	{
	}

	void prepare_draw() const noexcept override
	{
	}

	void child_update(float delta_time) noexcept override
	{
		if (spinning)
			_transform.rotate(glm::vec3(0.0f, spin_speed * delta_time, 0.0f));
	}
};