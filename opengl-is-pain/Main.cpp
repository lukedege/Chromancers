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

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/texture.h"
#include "utils/material.h"

#include "utils/scene/camera.h"
#include "utils/scene/entity.h"
#include "utils/scene/light.h "

#include "utils/window.h"

namespace ugl = utils::graphics::opengl;

#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3
#define MAX_DIR_LIGHTS   3
#define MAX_LIGHTS MAX_POINT_LIGHTS+MAX_SPOT_LIGHTS+MAX_DIR_LIGHTS

// helper functions
void set_light_attributes(ugl::Shader& shader);

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void process_pressed_keys();
void process_toggled_keys();

// input related parameters
float lastX, lastY;
bool firstMouse = true;
bool keys[1024];
bool capture_mouse = true;

// global scene camera
ugl::Camera camera;

// parameters for time computation
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// rotation speed on Y axis
float spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
bool spinning = true;

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

/////////////////// MAIN function ///////////////////////
int main()
{
	// Window creation
	ugl::window wdw
	{
		ugl::window::window_create_info
		{
			{ "Lighting test" }, //.title
			{ 4       }, //.gl_version_major
			{ 6       }, //.gl_version_minor
			{ 1280    }, //.window_width
			{ 720     }, //.window_height
			{ 1280    }, //.viewport_width
			{ 720     }, //.viewport_height
			{ true    }, //.resizable
			{ true    }, //.debug_gl
		}
	};
	
	GLFWwindow* glfw_window = wdw.get();
	ugl::window::window_size ws = wdw.get_size();
	float width = static_cast<float>(ws.width), height = static_cast<float>(ws.height);	
	
	// Callbacks linking with glfw
	glfwSetKeyCallback(glfw_window, key_callback);
	glfwSetCursorPosCallback(glfw_window, mouse_pos_callback);

	// Imgui setup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
	
	// Camera setup
	camera = ugl::Camera{ glm::vec3{0,0,5} };
	glm::mat4 view = camera.view_matrix();
	glm::mat4 projection = camera.projection_matrix();

	// Lights setup 
	ugl::PointLight pl1{ glm::vec3{-20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	ugl::PointLight pl2{ glm::vec3{ 20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	ugl::DirectionalLight dl1{glm::vec3{ 0, -1, -1 }, glm::vec4{1, 1, 1, 1}, 1 };

	std::vector<ugl::PointLight> point_lights;
	point_lights.push_back(pl1); point_lights.push_back(pl2);

	std::vector<ugl::DirectionalLight> dir_lights;
	dir_lights.push_back(dl1);

	currentLight = &point_lights[0];

	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	ugl::Shader floor_shader{ "shaders/text/scene/floor.vert", "shaders/text/scene/floor.frag", 4, 3, nullptr, utils_shaders};
	ugl::Shader basic_shader{ "shaders/text/generic/mvp.vert", "shaders/text/generic/basic.frag", 4, 3 };
	ugl::Shader parallax_map_shader{ "shaders/text/generic/BP_parallax_mapping.vert", "shaders/text/generic/BP_parallax_mapping.frag", 4, 3, nullptr, utils_shaders };

	std::vector <std::reference_wrapper<ugl::Shader>> lit_shaders;
	lit_shaders.push_back(floor_shader); lit_shaders.push_back(parallax_map_shader);

	// Textures setup
	ugl::Texture uv_tex{ "textures/UV_Grid_Sm.png" }, soil_tex { "textures/SoilCracked.png" };
	ugl::Texture wall_diffuse_tex{ "textures/brickwall.jpg" }, wall_normal_tex{ "textures/brickwall_normal.jpg" };
	ugl::Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" }, 
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex{ "textures/bricks2_disp.jpg" };
	
	// Materials
	ugl::Material redbricks_mat { parallax_map_shader, &redbricks_diffuse_tex, &redbricks_normal_tex, &redbricks_depth_tex };
	ugl::Material floor_mat { floor_shader, &wall_diffuse_tex, &wall_normal_tex, &redbricks_depth_tex };
	ugl::Material basic_mat { basic_shader };

	// Objects setup
	ugl::Model plane_model{ "models/plane.obj" }, cube_model{ "models/cube.obj" };
	ugl::Entity plane{ plane_model, floor_mat }, cube{ cube_model, redbricks_mat };

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
	ugl::Entity cursor{ triangle_mesh, basic_mat };

	std::vector<ugl::Entity*> scene_objects;
	scene_objects.push_back(&plane); scene_objects.push_back(&cube);
	
	// Shader "constants" setup
	for (ugl::Shader& lit_shader : lit_shaders)
	{
		set_light_attributes(lit_shader);
		lit_shader.bind();
		lit_shader.setUint("nPointLights", point_lights.size());
		lit_shader.setUint("nDirLights", dir_lights.size());
	}
	
	float norm_map_repeat = 90.f, parallax_map_repeat = 3.f;
	float parallax_heightscale = 0.05f;

	// Entities setup in scene
	plane.transform.set_position(glm::vec3(0.0f, -1.0f, 0.0f));
	plane.transform.set_size(glm::vec3(10.0f, 1.0f, 10.0f));

	cube.transform.set_position(glm::vec3(0.0f, 0.0f, 0.0f));
	cube.transform.set_size(glm::vec3(1.f));	// It's a bit too big for our scene, so scale it down

	cursor.transform.set_size(glm::vec3(3.0f));

	// Rendering loop: this code is executed at each frame
	while (wdw.is_open())
	{
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (capture_mouse)
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Check is an I/O event is happening
		glfwPollEvents();
		process_toggled_keys();
		process_pressed_keys();

		// Clear the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get matrices from camera
		view = camera.view_matrix();

		// Render objects with a window-wide viewport
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height); 

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

		// TODO move these into the overridden update method of a child of entity (e.g. Plane : Entity) or an "update" lambda within entity's scope (similar to command lists)
		#pragma region plane
		floor_mat.bind();
		floor_mat.shader->setMat4("projectionMatrix", projection);
		floor_mat.shader->setVec3("wCameraPos", camera.position());

		floor_mat.shader->setFloat("repeat", norm_map_repeat);
		floor_mat.shader->setFloat("height_scale", 0);

		plane.draw(view);
		#pragma endregion plane

		#pragma region cube
		if(spinning)
			cube.transform.rotate(glm::vec3(0.0f, spin_speed * deltaTime, 0.0f));
		
		redbricks_mat.bind();
		redbricks_mat.shader->setMat4("projectionMatrix", projection);
		redbricks_mat.shader->setVec3("wCameraPos", camera.position());

		redbricks_mat.shader->setFloat("repeat", parallax_map_repeat);
		redbricks_mat.shader->setFloat("height_scale", parallax_heightscale);

		cube.draw(view);

		#pragma endregion cube

		#pragma region map_draw
		// MAP
		glClear(GL_DEPTH_BUFFER_BIT); // clears depth information, thus everything rendered from now on will be on top https://stackoverflow.com/questions/5526704/how-do-i-keep-an-object-always-in-front-of-everything-else-in-opengl
		glViewport(ws.width - 200, ws.height - 150, 200, 150); // we render now into the smaller map viewport
		
		float topdown_height = 20;
		glm::vec3 new_cam_pos = camera.position() + glm::vec3{ 0, topdown_height, 0 };
		glm::mat4 topdown_view = glm::lookAt(new_cam_pos, camera.position(), camera.forward());
		
		// Redraw all scene objects from map pov
		for (ugl::Entity* o : scene_objects)
		{
			o->material->shader->bind(); // TODO make this better when u change stuff about camera, like calling their personal "update" method or lambda
			o->material->shader->setVec3("wCameraPos", new_cam_pos);
			o->draw(topdown_view);
		}
		
		// Prepare cursor shader
		glClear(GL_DEPTH_BUFFER_BIT);
		basic_shader.bind();
		basic_shader.setMat4("viewMatrix", topdown_view);
		basic_shader.setMat4("projectionMatrix", projection);

		cursor.transform.set_position(camera.position());
		//cursor.transform.rotate(camera.rotation().y + 90.f, { 0.0f, 0.0f, -1.0f });
		cursor.transform.set_rotation({ -90.0f, 0.0f, -camera.rotation().y - 90.f });

		cursor.draw(topdown_view);
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
			ImGui::SliderFloat("Repeat tex##norm", &norm_map_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Separator(); ImGui::Text("Parallax");
			ImGui::SliderFloat("Height Scale", &parallax_heightscale, 0, 0.5, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Repeat tex##prlx", &parallax_map_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		}
		if (ImGui::CollapsingHeader("Lights"))
		{
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				std::string light_label = "Intensity PL " + std::to_string(i);
				ImGui::SliderFloat(light_label.c_str(), &point_lights[i].intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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
	basic_shader.dispose();
	floor_shader.dispose();
	parallax_map_shader.dispose();
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

//////////////////////////////////////////

// HELPERS

void set_light_attributes(ugl::Shader& shader)
{
	shader.bind();
	shader.setVec3("ambient", ambient);
	shader.setVec3("diffuse", diffuse);
	shader.setVec3("specular", specular);

	shader.setFloat("kA", kA);
	shader.setFloat("kD", kD);
	shader.setFloat("kS", kS);

	shader.setFloat("shininess", shininess);
	shader.setFloat("alpha", alpha);
}

// INPUT
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// if ESC is pressed, we close the application
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

// Process and release 
void process_toggled_keys()
{
	if (keys[GLFW_KEY_LEFT_ALT])
	{
		capture_mouse = !capture_mouse;
		keys[GLFW_KEY_LEFT_ALT] = false;
	}
	if (keys[GLFW_KEY_P])
	{
		spinning = !spinning;
		keys[GLFW_KEY_P] = false;
	}
	if (keys[GLFW_KEY_L])
	{
		wireframe = !wireframe;
		if (wireframe)
			// Draw in wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		keys[GLFW_KEY_L] = false;
	}
}

// Process until user release
void process_pressed_keys()
{
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(ugl::Camera::Directions::FORWARD , deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(ugl::Camera::Directions::BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(ugl::Camera::Directions::LEFT    , deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(ugl::Camera::Directions::RIGHT   , deltaTime);
	if (keys[GLFW_KEY_Q])
		camera.ProcessKeyboard(ugl::Camera::Directions::DOWN, deltaTime);
	if (keys[GLFW_KEY_E])
		camera.ProcessKeyboard(ugl::Camera::Directions::UP, deltaTime);
	if (keys[GLFW_KEY_SPACE])
		camera.toggle_fly();

	if (keys[GLFW_KEY_LEFT])
		currentLight->position.x -= mov_light_speed * deltaTime;
	if (keys[GLFW_KEY_RIGHT])
		currentLight->position.x += mov_light_speed * deltaTime;
	if (keys[GLFW_KEY_UP])
		currentLight->position.z -= mov_light_speed * deltaTime;
	if (keys[GLFW_KEY_DOWN])
		currentLight->position.z += mov_light_speed * deltaTime;
}

void mouse_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
	float x_posf = static_cast<float>(x_pos), y_posf = static_cast<float>(y_pos);
	if (firstMouse)
	{
		lastX = x_posf;
		lastY = y_posf;
		firstMouse = false;
	}

	float x_offset = x_posf - lastX;
	float y_offset = lastY - y_posf;

	lastX = x_posf;
	lastY = y_posf;

	if(capture_mouse)
		camera.ProcessMouseMovement(x_offset, y_offset);
}