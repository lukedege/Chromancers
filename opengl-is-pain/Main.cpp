// std libraries
#include <string>
#include <functional>

#include <gsl/gsl>

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
#include "utils/framebuffer.h"

#include "utils/scene/light.h "
#include "utils/scene/camera.h"
#include "utils/scene/scene.h "
#include "utils/scene/entity.h"

#include "utils/components/rigidbody_component.h"

#include "utils/window.h"

// entities
#include "entities/floor.h"
#include "entities/cube.h"

namespace
{
	using namespace engine::physics;
	using namespace engine::scene;
	using namespace engine::components;
	using namespace engine::input;
	using namespace engine::resources;
	using namespace utils::graphics::opengl;
}

// window
window* wdw_ptr;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void setup_input_keys();


// input related parameters
float cursor_x, cursor_y;
bool firstMouse = true;
bool capture_mouse = true;

// global scene camera
Camera main_camera;
Camera topdown_camera;
std::function<void()> scene_setup;

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
PointLight* currentLight;
float mov_light_speed = 30.f;

// Scene material/lighting setup 
glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
float kD = 0.5f, kS = 0.4f, kA = 0.1f; // Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
float shininess = 25.f;
float alpha = 0.2f;
float F0 = 0.9f;

// Physics
PhysicsEngine physics_engine;
float maxSecPerFrame = 1.0f / 60.0f;
float capped_deltaTime;

// Temporary 
Entity* sphere_ptr;


// TODOs 
// - Spawn multiple spheres with rigidbodies

/////////////////// MAIN function ///////////////////////
int main()
{
	window wdw
	{
		window::window_create_info
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

	wdw_ptr = &wdw;

	GLFWwindow* glfw_window = wdw.get();
	window::window_size ws = wdw.get_size();
	float width = gsl::narrow<float>(ws.width), height = gsl::narrow<float>(ws.height);
	
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

#pragma region scene_setup	
	// Camera setup
	main_camera = Camera{ glm::vec3{0,0,5} };
	glm::mat4 view = main_camera.viewMatrix();
	glm::mat4 projection = main_camera.projectionMatrix();

	// Lights setup 
	PointLight pl1{ glm::vec3{-20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	PointLight pl2{ glm::vec3{ 20.f, 10.f, 10.f}, glm::vec4{1, 1, 1, 1}, 1 };
	DirectionalLight dl1{glm::vec3{ 0, -1, -1 }, glm::vec4{1, 1, 1, 1}, 1 };

	std::vector<PointLight> point_lights;
	point_lights.push_back(pl1); point_lights.push_back(pl2);

	std::vector<DirectionalLight> dir_lights;
	dir_lights.push_back(dl1);

	currentLight = &point_lights[0];

	// Scene setup
	SceneData scene_data;
	scene_data.current_camera = &main_camera;

#pragma endregion shader_setup

#pragma region shader_setup
	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	Shader basic_shader     { "shaders/text/generic/basic.vert" ,"shaders/text/generic/basic.frag", 4, 3 };
	Shader basic_mvp_shader { "shaders/text/generic/mvp.vert", "shaders/text/generic/basic.frag", 4, 3 };
	Shader debug_shader     { "shaders/text/generic/mvp.vert", "shaders/text/generic/fullcolor.frag", 4, 3 };
	Shader default_lit      { "shaders/text/default_lit.vert", "shaders/text/default_lit.frag", 4, 3, nullptr, utils_shaders };
	Shader textured_shader  { "shaders/text/generic/textured.vert" , "shaders/text/generic/textured.frag", 4, 3 };
	Shader tex_depth_shader { "shaders/text/generic/textured.vert" , "shaders/text/generic/textured_depth.frag", 4, 3 };
	Shader shadowmap_shader { "shaders/text/generic/shadow_map.vert" , "shaders/text/generic/shadow_map.frag", 4, 3 };

	std::vector <std::reference_wrapper<Shader>> lit_shaders, all_shaders;
	lit_shaders.push_back(default_lit);
	all_shaders.push_back(basic_mvp_shader); all_shaders.push_back(default_lit);

	// Shader commons and "constants" setup
	for (Shader& shader : all_shaders)
	{
		shader.bind();
		shader.setMat4("projectionMatrix", projection);
		shader.setVec3("wCameraPos", main_camera.position());
	}
	for (Shader& lit_shader : lit_shaders)
	{
		lit_shader.bind();
		lit_shader.setUint("nPointLights", gsl::narrow<unsigned int>(point_lights.size()));
		lit_shader.setUint("nDirLights"  , gsl::narrow<unsigned int>(dir_lights.size()));
	}
#pragma endregion shader_setup

#pragma region materials_setup
	// Textures setup
	Texture floor_diffuse_tex{ "textures/brickwall.jpg" }, floor_normal_tex{ "textures/brickwall_normal.jpg" };
	Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" }, 
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex { "textures/bricks2_disp.jpg" };
	
	// Materials
	Material redbricks_mat { default_lit };
	redbricks_mat.diffuse_map = &redbricks_diffuse_tex; redbricks_mat.normal_map = &redbricks_normal_tex; redbricks_mat.displacement_map = &redbricks_depth_tex;
	redbricks_mat.uv_repeat = 3.f; redbricks_mat.parallax_heightscale = 0.05f;

	Material redbricks_mat_2 { default_lit };
	redbricks_mat_2.diffuse_map = &redbricks_diffuse_tex; redbricks_mat_2.normal_map = &redbricks_normal_tex; redbricks_mat_2.displacement_map = &redbricks_depth_tex;
	redbricks_mat_2.uv_repeat = 20.f; redbricks_mat_2.parallax_heightscale = 0.05f;

	Material floor_mat { default_lit };
	floor_mat.diffuse_map = &floor_diffuse_tex; floor_mat.normal_map = &floor_normal_tex;
	// LA UV REPEAT E I SUOI ESTREMI DEVONO DIPENDERE DALLA DIMENSIONE DELLA TEXTURE E/O DALLA SIZE DELLA TRANSFORM?
	floor_mat.uv_repeat = 0.75f * std::max(floor_mat.diffuse_map->width(), floor_mat.diffuse_map->height()); 

	Material sph_mat { default_lit };

	Material basic_mat { basic_mvp_shader };
#pragma endregion materials_setup

	// Entities setup
	Model plane_model{ "models/quad.obj" }, cube_model{ "models/cube.obj" }, sphere_model{ "models/sphere.obj" }, bunny_model{ "models/bunny.obj" };
	Entity cube        { "cube", cube_model , redbricks_mat, scene_data };
	Entity floor_plane { "floor", plane_model, floor_mat, scene_data };
	Entity wall_plane  { "wall", plane_model, redbricks_mat_2, scene_data };
	Entity sphere      { "sphere", sphere_model, sph_mat, scene_data };
	Entity bunny       { "bunny", bunny_model, sph_mat, scene_data };
	sphere_ptr = &sphere;

	Model triangle_mesh { Mesh::simple_triangle_mesh() };
	Model quad_mesh{ Mesh::simple_quad_mesh() };
	Entity cursor{ "cursor", triangle_mesh, basic_mat, scene_data};

	std::vector<Entity*> scene_objects;
	scene_objects.push_back(&floor_plane); scene_objects.push_back(&wall_plane);
	scene_objects.push_back(&cube); scene_objects.push_back(&sphere);
	scene_objects.push_back(&bunny);

	// Entities setup in scene
	scene_setup = [&]()
	{
		floor_plane.set_position(glm::vec3(0.0f, -1.0f, 0.0f));
		floor_plane.set_size(glm::vec3(100.0f, 0.1f, 100.0f));

		wall_plane.set_position(glm::vec3(0.0f, 4.0f, -10.0f));
		wall_plane.set_rotation(glm::vec3(90.0f, 0.0f, 0.f));
		wall_plane.set_size(glm::vec3(10.0f, 0.1f, 5.0f));

		cube.set_position(glm::vec3(0.0f, 3.0f, 0.0f));
		cube.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));

		sphere.set_position(glm::vec3(0.0f, 0.0f, 0.0f));
		sphere.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
		sphere.set_size(glm::vec3(0.25f));

		bunny.set_position(glm::vec3(-5.0f, 3.0f, 0.0f));
		bunny.set_size(glm::vec3(0.5f));

		cursor.set_size(glm::vec3(3.0f));
	};
	scene_setup();
	

	// Physics setup
	GLDebugDrawer phy_debug_drawer{main_camera, debug_shader};
	physics_engine.addDebugDrawer(&phy_debug_drawer);
	physics_engine.set_debug_mode(1);

	RigidBodyComponent fpl_rb{ floor_plane, physics_engine, PhysicsEngine::RigidBodyCreateInfo{ PhysicsEngine::ColliderShape::BOX,    glm::vec3{100.0f, 0.01f, 100.0f}, 0.0f, 3.0f, 0.5f}, false };
	RigidBodyComponent wpl_rb{ wall_plane , physics_engine, PhysicsEngine::RigidBodyCreateInfo{ PhysicsEngine::ColliderShape::BOX,    glm::vec3{1}, 0.0f, 3.0f, 0.5f}, true };
	RigidBodyComponent cub_rb{ cube       , physics_engine, PhysicsEngine::RigidBodyCreateInfo{ PhysicsEngine::ColliderShape::BOX,    glm::vec3{1}, 1.0f, 0.1f, 0.1f}, true };
	RigidBodyComponent sph_rb{ sphere     , physics_engine, PhysicsEngine::RigidBodyCreateInfo{ PhysicsEngine::ColliderShape::SPHERE, glm::vec3{1}, 1.0f, 1.0f, 1.0f}, true };
	RigidBodyComponent bun_rb{ bunny      , physics_engine, PhysicsEngine::RigidBodyCreateInfo{ PhysicsEngine::ColliderShape::BOX,    glm::vec3{2}, 1.0f, 1.0f, 1.0f}, false };
	floor_plane.components.push_back(&fpl_rb);
	wall_plane .components.push_back(&wpl_rb);
	cube       .components.push_back(&cub_rb);
	sphere     .components.push_back(&sph_rb);
	bunny      .components.push_back(&bun_rb);

	// Framebuffers
	Framebuffer map_framebuffer { ws.width, ws.height };
	Framebuffer shadows_framebuffer { 1024, 1024 };

	// Shadow map setup
	// (for now only the first dir light is a shadowcaster)
	float near_plane = 1.f, far_plane = 20.f, frustum_size = 20.f;
	glm::mat4 lightProjection = glm::ortho(-frustum_size, frustum_size, -frustum_size, frustum_size, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(-dir_lights[0].direction * 5.f, glm::vec3(0.f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	// Rendering loop: this code is executed at each frame
	while (wdw.is_open())
	{
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		float currentFrame = gsl::narrow_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		capped_deltaTime = deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame;
		lastFrame = currentFrame;

		if (capture_mouse)
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Check is an I/O event is happening
		glfwPollEvents();
		Input::instance().process_pressed_keys();

		// Clear the frame and z buffer
		glClearColor(0.26f, 0.46f, 0.98f, 1.0f); //the "clear" color for the default frame buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		scene_data.current_camera = &main_camera;

		// Render objects with a window-wide viewport
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height); 

		// Update commons for all shaders (better in a common loop or done in entity like now?)
		for (Shader& shader : all_shaders)
		{
			shader.bind();
			shader.setVec3("wCameraPos", scene_data.current_camera->position());
			shader.setMat4("viewMatrix", scene_data.current_camera->viewMatrix());
		}

		// Update lighting for lit shaders
		for (Shader& lit_shader : lit_shaders)
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
			lit_shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		} 

		// Update physics simulation
		physics_engine.step(capped_deltaTime);
		physics_engine.detect_collisions();

		// Update entities
		for (Entity* o : scene_objects)
		{
			o->update(capped_deltaTime);
		}

		#pragma region shadow_pass
		// Shadow pass
		shadows_framebuffer.bind();
		glClear(GL_DEPTH_BUFFER_BIT);

		shadowmap_shader.bind();
		shadowmap_shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		for (Entity* o : scene_objects)
		{
			shadowmap_shader.setMat4("modelMatrix", o->transform().world_matrix());
			o->plain_draw();
		}

		shadows_framebuffer.unbind();

		
		//debug draw the depth texture
		glViewport(0, 0, 256, 256);
		tex_depth_shader.bind();
		glActiveTexture(GL_TEXTURE0);
		shadows_framebuffer.get_depth_attachment().bind();
		quad_mesh.draw();
		textured_shader.unbind();
		glViewport(0, 0, ws.width, ws.height);
		

		#pragma endregion shadow_pass

		// Render scene
		for (Entity* o : scene_objects)
		{
			glActiveTexture(GL_TEXTURE4); // temp
			o->material->shader->bind();
			shadows_framebuffer.get_depth_attachment().bind();
			o->material->shader->setInt("shadow_map", 4);
			o->draw();
		}
		
		#pragma region map_draw
		// MAP
		map_framebuffer.bind();
		glClearColor(0.5f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float topdown_height = 20;
		topdown_camera.set_position(main_camera.position() + glm::vec3{ 0, topdown_height, 0 });
		topdown_camera.lookAt(main_camera.position());
		scene_data.current_camera = &topdown_camera;

		// Update camera info since we swapped to topdown
		for (Shader& shader : all_shaders)
		{
			shader.bind();
			shader.setVec3("wCameraPos", scene_data.current_camera->position());
			shader.setMat4("viewMatrix", scene_data.current_camera->viewMatrix());
		}
		
		// Redraw all scene objects from map pov
		for (Entity* o : scene_objects)
		{
			o->draw();
		}
		
		// Prepare cursor shader
		glClear(GL_DEPTH_BUFFER_BIT); // clears depth information, thus everything rendered from now on will be on top https://stackoverflow.com/questions/5526704/how-do-i-keep-an-object-always-in-front-of-everything-else-in-opengl

		cursor.set_position(main_camera.position());
		cursor.set_rotation({ -90.0f, 0.0f, -main_camera.rotation().y - 90.f });

		cursor.update(deltaTime);
		cursor.draw();

		map_framebuffer.unbind();

		// we render now into the smaller map viewport
		glViewport(ws.width - map_framebuffer.width() / 4 - 10, ws.height - map_framebuffer.height() / 4 - 10, map_framebuffer.width() / 4, map_framebuffer.height() / 4);
		textured_shader.bind();
		glActiveTexture(GL_TEXTURE0);
		map_framebuffer.get_color_attachment().bind();
		quad_mesh.draw();
		textured_shader.unbind();
		glViewport(0, 0, ws.width, ws.height);
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
			ImGui::SliderFloat("Repeat tex##norm", &floor_plane.material->uv_repeat, 0, 3000, " % .1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Separator(); ImGui::Text("Parallax");
			ImGui::SliderFloat("Height Scale", &cube.material->parallax_heightscale, 0, 0.5, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Repeat tex##prlx", &cube.material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		}
		if (ImGui::CollapsingHeader("Lights"))
		{
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				ImGui::PushID(&point_lights+i);
				std::string light_label = "Point light n." + std::to_string(i);
				ImGui::Separator(); ImGui::Text(light_label.c_str());
				
				ImGui::SliderFloat("Intensity", &point_lights[i].intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::ColorEdit4("Color", glm::value_ptr(point_lights[i].color));

				ImGui::PopID();
			}
			for (size_t i = 0; i < dir_lights.size(); i++)
			{
				ImGui::PushID(&dir_lights+i);
				std::string light_label = "Directional light n." + std::to_string(i);
				ImGui::Separator(); ImGui::Text(light_label.c_str());

				ImGui::SliderFloat("Intensity", &dir_lights[i].intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::ColorEdit4("Color", glm::value_ptr(dir_lights[i].color));

				ImGui::PopID();
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
	for (Shader& shader : all_shaders)
	{
		shader.dispose();
	}
	
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
	Input::instance().add_onPressed_callback(GLFW_KEY_W, [&]() { main_camera.ProcessKeyboard(Camera::Directions::FORWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_S, [&]() { main_camera.ProcessKeyboard(Camera::Directions::BACKWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_A, [&]() { main_camera.ProcessKeyboard(Camera::Directions::LEFT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_D, [&]() { main_camera.ProcessKeyboard(Camera::Directions::RIGHT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_Q, [&]() { main_camera.ProcessKeyboard(Camera::Directions::DOWN, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_E, [&]() { main_camera.ProcessKeyboard(Camera::Directions::UP, deltaTime); });

	Input::instance().add_onPressed_callback(GLFW_KEY_LEFT , [&]() { currentLight->position.x -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_RIGHT, [&]() { currentLight->position.x += mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_UP   , [&]() { currentLight->position.z -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_DOWN , [&]() { currentLight->position.z += mov_light_speed * deltaTime; });

	Input::instance().add_onPressed_callback(GLFW_KEY_F, [&]()
		{
			window::window_size ws = wdw_ptr->get_size();
			glm::vec4 wCursorPosition = utils::math::unproject(cursor_x, cursor_y, ws.width, ws.height, main_camera.viewMatrix(), main_camera.projectionMatrix());

			// TODO modify stuff based on cursor position
		});

	// Toggled input
	Input::instance().add_onRelease_callback(GLFW_KEY_ESCAPE  , [&]() { wdw_ptr->close(); });
	Input::instance().add_onRelease_callback(GLFW_KEY_LEFT_ALT, [&]() { capture_mouse = !capture_mouse; });
	Input::instance().add_onRelease_callback(GLFW_KEY_SPACE   , [&]() { main_camera.toggle_fly(); });
	Input::instance().add_onRelease_callback(GLFW_KEY_P, [&]() { spinning = !spinning; });
	Input::instance().add_onRelease_callback(GLFW_KEY_R, [&]() { scene_setup(); });
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
			sphere_ptr->set_position(main_camera.position() + main_camera.forward());

			float shootInitialSpeed = 20.f;
			glm::vec3 shoot_dir = main_camera.forward() * shootInitialSpeed;
			btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
			btRigidBody* rb = static_cast<RigidBodyComponent*>(sphere_ptr->components[0])->rigid_body;// temp
			rb->applyCentralImpulse(impulse); // temp
		});
}

void mouse_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
	float x_posf = gsl::narrow<float>(x_pos), y_posf = gsl::narrow<float>(y_pos);
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
