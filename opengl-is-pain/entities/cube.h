#pragma once

#include "../utils/model.h"
#include "../utils/material.h"
#include "../utils/scene/entity.h"
#include "../utils/scene/scene.h"

using namespace utils::graphics::opengl;

class Cube : public Entity
{
public:
	float parallax_map_repeat = 3.f;
	float parallax_heightscale = 0.05f;

	float spin_speed = 30.f;
	bool spinning = true;

	Cube(Model& drawable, Material& material, SceneData& current_scene) :
		Entity{ drawable, material, current_scene }
	{}

	void prepare_draw() const noexcept
	{
		material->bind();

		material->shader->setFloat("repeat", parallax_map_repeat);
		material->shader->setFloat("height_scale", parallax_heightscale);

		if (current_scene)
		{
			material->shader->setVec3("wCameraPos", current_scene->current_camera->position());
		}

		material->unbind();
	}

	void child_update(float delta_time) noexcept
	{
		if (spinning && is_kinematic)
			transform.rotate(glm::vec3(0.0f, spin_speed * delta_time, 0.0f));
	}
};