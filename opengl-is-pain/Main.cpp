// std libraries
#include <string>
#include <functional>

// GL libraries
#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// macros
#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3
#define MAX_DIR_LIGHTS   3
#define MAX_LIGHTS MAX_POINT_LIGHTS+MAX_SPOT_LIGHTS+MAX_DIR_LIGHTS
//#define DEBUG_UNIFORM

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/texture.h"
#include "utils/material.h"
#include "utils/physics.h"
#include "utils/input.h"

#include "utils/scene/light.h "
#include "utils/scene/camera.h"
#include "utils/scene/scene.h "
#include "utils/scene/entity.h"

#include "utils/window.h"

// entities
#include "entities/floor.h"
#include "entities/cube.h"

namespace ugl = utils::graphics::opengl;

// window
ugl::window wdw
{
	ugl::window::window_create_info
	{
		{ "Lighting test" }, //.title
		{ 4 }, //.gl_version_major
		{ 6 }, //.gl_version_minor
		{ 1280 }, //.window_width
		{ 720 }, //.window_height
		{ 1280 }, //.viewport_width
		{ 720 }, //.viewport_height
		{ true }, //.resizable
		{ true }, //.debug_gl
	}
};

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void setup_input_keys();

// input related parameters
float cursor_x, cursor_y;
bool firstMouse = true;
bool capture_mouse = true;

// global scene camera
ugl::Camera main_camera;
ugl::Camera topdown_camera;

// parameters for time computation
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// rotation speed on Y axis
float spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
bool spinning = false;

// boolean to activate/deactivate wireframe rendering
bool wireframe = false;

// Lights
ugl::PointLight* currentLight;
float mov_light_speed = 30.f;

// Scene material/lighting setup 
glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
float kD = 0.5f, kS = 0.4f, kA = 0.1f; // Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
float shininess = 25.f;
float alpha = 0.2f;
float F0 = 0.9f;

// Physics
ugl::Physics physics_engine;
float maxSecPerFrame = 1.0f / 60.0f;
float capped_deltaTime;

// Temporary 
Entity* sphere;


// TODOs
// - Spawn multiple spheres with rigidbodies
// - Make a "Physics_entity" class which is the same as entity but involves a rigidbody as default

/////////////////// MAIN function ///////////////////////
int main()
{
	GLFWwindow* glfw_window = wdw.get();
	ugl::window::window_size ws = wdw.get_size();
	float width = static_cast<float>(ws.width), height = static_cast<float>(ws.height);	
	
	// Callbacks linking with glfw
	glfwSetKeyCallback(glfw_window, key_callback);
	glfwSetCursorPosCallback(glfw_window, mouse_pos_callback);
	
	// Setup keys
	setup_input_keys();

	// Imgui setup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
	
	// Camera setup
	main_camera = ugl::Camera{ glm::vec3{0,0,5} };
	glm::mat4 view = main_camera.viewMatrix();
	glm::mat4 projection = main_camera.projectionMatrix();

	// Lights setup 
	ugl::PointLight pl1{ glm::vec3{-20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	ugl::PointLight pl2{ glm::vec3{ 20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	ugl::DirectionalLight dl1{glm::vec3{ 0, -1, -1 }, glm::vec4{1, 1, 1, 1}, 1 };

	std::vector<ugl::PointLight> point_lights;
	point_lights.push_back(pl1); point_lights.push_back(pl2);

	std::vector<ugl::DirectionalLight> dir_lights;
	dir_lights.push_back(dl1);

	currentLight = &point_lights[0];

	// Scene setup
	ugl::SceneData scene_data;
	scene_data.current_camera = &main_camera;
	scene_data.physics_engine = &physics_engine;

	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	ugl::Shader basic_shader{ "shaders/text/generic/mvp.vert", "shaders/text/generic/basic.frag", 4, 3 };
	ugl::Shader default_lit{ "shaders/text/default_lit.vert", "shaders/text/default_lit.frag", 4, 3, nullptr, utils_shaders };

	std::vector <std::reference_wrapper<ugl::Shader>> lit_shaders, all_shaders;
	lit_shaders.push_back(default_lit);
	all_shaders.push_back(basic_shader); all_shaders.push_back(default_lit);

	// Textures setup
	ugl::Texture wall_diffuse_tex{ "textures/brickwall.jpg" }, wall_normal_tex{ "textures/brickwall_normal.jpg" };
	ugl::Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" }, 
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex{ "textures/bricks2_disp.jpg" };
	
	// Materials
	ugl::Material redbricks_mat { default_lit };
	redbricks_mat.diffuse_map = &redbricks_diffuse_tex; redbricks_mat.normal_map = &redbricks_normal_tex; redbricks_mat.displacement_map = &redbricks_depth_tex;
	redbricks_mat.uv_repeat = 3.f; redbricks_mat.parallax_heightscale = 0.05f;

	ugl::Material floor_mat { default_lit };
	floor_mat.diffuse_map = &wall_diffuse_tex; floor_mat.normal_map = &wall_normal_tex;
	floor_mat.uv_repeat = 80.f;

	ugl::Material sph_mat { default_lit };

	ugl::Material basic_mat { basic_shader };

	// Objects setup
	ugl::Model plane_model{ "models/plane.obj" }, cube_model{ "models/cube.obj" }, sphere_model{ "models/sphere.obj" };
	Cube cube{ cube_model, redbricks_mat, scene_data };
	Floor floor_plane{ plane_model, floor_mat, scene_data, 90.f };
	ugl::Entity sph { sphere_model, sph_mat, scene_data };
	sphere = &sph;

	ugl::Model triangle_mesh
	{
		ugl::Mesh
		{
			std::vector<ugl::Vertex>
			{
				ugl::Vertex{ glm::vec3{-0.5f, -0.5f, 0.0f} /* position */ },
				ugl::Vertex{ glm::vec3{ 0.0f,  0.5f, 0.0f} /* position */ },
				ugl::Vertex{ glm::vec3{ 0.5f, -0.5f, 0.0f} /* position */ }
			},
				std::vector<GLuint>{0, 2, 1}
		}
	};
	ugl::Entity cursor{ triangle_mesh, basic_mat, scene_data };

	std::vector<ugl::Entity*> scene_objects;
	scene_objects.push_back(&floor_plane); scene_objects.push_back(&cube);
	
	// Shader commons and "constants" setup
	for (ugl::Shader& shader : all_shaders)
	{
		shader.bind();
		shader.setMat4("projectionMatrix", projection);
		shader.setVec3("wCameraPos", main_camera.position());
	}
	for (ugl::Shader& lit_shader : lit_shaders)
	{
		lit_shader.bind();
		lit_shader.setUint("nPointLights", point_lights.size());
		lit_shader.setUint("nDirLights", dir_lights.size());
	}
	
	float norm_map_repeat = 90.f, parallax_map_repeat = 3.f;
	float parallax_heightscale = 0.05f;

	// Entities setup in scene
	floor_plane.transform.set_position(glm::vec3(0.0f, -1.0f, 0.0f));
	floor_plane.transform.set_size(glm::vec3(10.0f, 0.1f, 10.0f));

	cube.transform.set_position(glm::vec3(0.0f, 3.0f, 0.0f));
	cube.transform.set_size(glm::vec3(1.f));	// It's a bit too big for our scene, so scale it down

	cursor.transform.set_size(glm::vec3(3.0f));

	// Physics setup
	floor_plane.add_rigidbody(ugl::Physics::RigidBodyCreateInfo{ ugl::Physics::ColliderShape::BOX, 0.f, 3.0f, 0.5f});
	cube.add_rigidbody(ugl::Physics::RigidBodyCreateInfo{ ugl::Physics::ColliderShape::BOX, 1.f, 0.1f, 0.1f});
	sph.add_rigidbody(ugl::Physics::RigidBodyCreateInfo{ ugl::Physics::ColliderShape::SPHERE, 1.f, 1.0f, 1.0f});

	// Rendering loop: this code is executed at each frame
	while (wdw.is_open())
	{
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		capped_deltaTime = deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame;
		lastFrame = currentFrame;

		if (capture_mouse)
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Check is an I/O event is happening
		glfwPollEvents();
		//process_toggled_keys();
		//process_pressed_keys();
		Input::instance().process_pressed_keys();

		// Clear the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get matrices from camera
		scene_data.current_camera = &main_camera;

		// Render objects with a window-wide viewport
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height); 

		// TODO Update commons for all shaders (better in a common loop or done in entity like now?)
		//for (ugl::Shader& shader : all_shaders)
		//{
		//	shader.bind();
		//	shader.setVec3("wCameraPos", main_camera.position());
		//}

		// Update lighting for lit shaders
		for (ugl::Shader& lit_shader : lit_shaders)
		{
			lit_shader.bind();
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				point_lights[i].setup(lit_shader, i);
			}
			for (size_t i = 0; i < dir_lights.size(); i++)
			{
				dir_lights[i].setup(lit_shader, i);
			}
		}

		// Update physics simulation
		physics_engine.dynamicsWorld->stepSimulation(capped_deltaTime, 10);

		floor_plane.update(capped_deltaTime);
		floor_plane.draw();

		cube.spinning = spinning;
		cube.update(capped_deltaTime);
		cube.draw();

		sph.update(capped_deltaTime);
		sph.draw();
		
		#pragma region map_draw
		// MAP
		glClear(GL_DEPTH_BUFFER_BIT); // clears depth information, thus everything rendered from now on will be on top https://stackoverflow.com/questions/5526704/how-do-i-keep-an-object-always-in-front-of-everything-else-in-opengl
		glViewport(ws.width - 200, ws.height - 150, 200, 150); // we render now into the smaller map viewport
		
		float topdown_height = 20;
		topdown_camera.set_position(main_camera.position() + glm::vec3{ 0, topdown_height, 0 });
		topdown_camera.lookAt(main_camera.position());
		scene_data.current_camera = &topdown_camera;
		
		// Redraw all scene objects from map pov
		for (ugl::Entity* o : scene_objects)
		{
			o->draw();
		}
		
		// Prepare cursor shader
		glClear(GL_DEPTH_BUFFER_BIT);

		cursor.transform.set_position(main_camera.position());
		cursor.transform.set_rotation({ -90.0f, 0.0f, -main_camera.rotation().y - 90.f });

		cursor.update(deltaTime);
		cursor.draw();
		#pragma endregion map_draw
		
		#pragma region imgui_draw
		// ImGUI window creation
		ImGui_ImplOpenGL3_NewFrame();// Tell OpenGL a new Imgui frame is about to begin
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Settings");
		
		if (ImGui::CollapsingHeader("Coefficients and scales"))
		{
			ImGui::Separator(); ImGui::Text("Normal");
			ImGui::SliderFloat("Repeat tex##norm", &floor_plane.material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Separator(); ImGui::Text("Parallax");
			ImGui::SliderFloat("Height Scale", &cube.material->parallax_heightscale, 0, 0.5, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Repeat tex##prlx", &cube.material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		}
		if (ImGui::CollapsingHeader("Lights"))
		{
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				std::string light_label = "Intensity PL " + std::to_string(i);
				ImGui::SliderFloat(light_label.c_str(), &point_lights[i].intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			}
			for (size_t i = 0; i < dir_lights.size(); i++)
			{
				std::string light_label = "Intensity DL " + std::to_string(i);
				ImGui::SliderFloat(light_label.c_str(), &dir_lights[i].intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			}
		}

		ImGui::End();

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		#pragma endregion imgui_draw

		// Swap buffers
		glfwSwapBuffers(glfw_window);
	}

	// Cleanup
	for (ugl::Shader& shader : all_shaders)
	{
		shader.dispose();
	}
	physics_engine.Clear();
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

//////////////////////////////////////////

// HELPERS

// INPUT
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (action == GLFW_PRESS)
	{
		Input::instance().press_key(key);
	}
	else if (action == GLFW_RELEASE)
	{
		Input::instance().release_key(key);
	}
}

void setup_input_keys()
{
	// Pressed input
	Input::instance().add_onPressed_callback(GLFW_KEY_H, [&]() { std::cout << "h pressed\n"; }); // TEMP
	Input::instance().add_onRelease_callback(GLFW_KEY_H, [&]() { std::cout << "h released\n"; }); // TEMP
	Input::instance().add_onPressed_callback(GLFW_KEY_W, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::FORWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_S, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::BACKWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_A, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::LEFT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_D, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::RIGHT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_Q, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::DOWN, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_E, [&]() { main_camera.ProcessKeyboard(ugl::Camera::Directions::UP, deltaTime); });

	Input::instance().add_onPressed_callback(GLFW_KEY_LEFT , [&]() { currentLight->position.x -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_RIGHT, [&]() { currentLight->position.x += mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_UP   , [&]() { currentLight->position.z -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_DOWN , [&]() { currentLight->position.z += mov_light_speed * deltaTime; });

	Input::instance().add_onPressed_callback(GLFW_KEY_F, [&]()
		{
			ugl::window::window_size ws = wdw.get_size();
			glm::vec4 wCursorPosition = ugl::unproject(cursor_x, cursor_y, ws.width, ws.height, main_camera.viewMatrix(), main_camera.projectionMatrix());

			// TODO modify stuff based on cursor position
		});

	// Toggled input
	Input::instance().add_onRelease_callback(GLFW_KEY_ESCAPE, [&]() { wdw.close(); });
	Input::instance().add_onRelease_callback(GLFW_KEY_LEFT_ALT, [&]() { capture_mouse = !capture_mouse; });
	Input::instance().add_onRelease_callback(GLFW_KEY_SPACE   , [&]() { main_camera.toggle_fly(); });
	Input::instance().add_onRelease_callback(GLFW_KEY_P, [&]() { spinning = !spinning; });
	Input::instance().add_onRelease_callback(GLFW_KEY_L, [&]() 
		{ 
			wireframe = !wireframe; 
			if (wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);// Draw in wireframe
			else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);// Fill in fragments
		});
	Input::instance().add_onRelease_callback(GLFW_KEY_X, [&]()
		{
			sphere->teleport_at(main_camera.position());

			float shootInitialSpeed = 20.f;
			glm::vec3 shoot_dir = main_camera.forward() * shootInitialSpeed;
			btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
			sphere->rigid_body->applyCentralImpulse(impulse);
		});
}

void mouse_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
	float x_posf = static_cast<float>(x_pos), y_posf = static_cast<float>(y_pos);
	if (firstMouse)
	{
		cursor_x = x_posf;
		cursor_y = y_posf;
		firstMouse = false;
	}

	float x_offset = x_posf - cursor_x;
	float y_offset = cursor_y - y_posf;

	cursor_x = x_posf;
	cursor_y = y_posf;

	if(capture_mouse)
		main_camera.ProcessMouseMovement(x_offset, y_offset);
}