#pragma once

#include <vector>
#include "camera.h"
#include "light.h"
#include "../physics.h"

namespace utils::graphics::opengl
{
	struct SceneData
	{
		Camera* current_camera;
		std::vector<Light*> lights;
		Physics* physics_engine;
	};

	//class Scene
	//{
	//private:
	//	std::vector<Entity*> entities;  //TODO for now vector is sufficient, a tree or a better container would be optimal (also for now pointers are sufficient)
	//public:
	//	Camera* main_camera;
	//	std::vector<Light*> lights;
	//
	//	// TODO
	//};
}