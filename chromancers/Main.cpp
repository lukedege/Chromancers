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

#include <implot.h>

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/texture.h"
#include "utils/material.h"
#include "utils/physics.h"
#include "utils/input.h"
#include "utils/framebuffer.h"
#include "utils/random.h"

#include "utils/scene/camera.h "
#include "utils/scene/entity.h "
#include "utils/scene/scene.h  "
#include "utils/scene/light.h  "
#include "utils/scene/player.h "

#include "utils/components/rigidbody_component.h"
#include "utils/components/paintable_component.h"
#include "utils/components/paintball_component.h"
#include "utils/components/paintball_spawner_component.h"

#include "utils/window.h"

namespace
{
	using namespace engine::physics;
	using namespace engine::scene;
	using namespace engine::components;
	using namespace engine::input;
	using namespace engine::resources;
	using namespace utils::graphics::opengl;
}

// Window
Window* wdw_ptr;
Window::window_size ws;

// Framebuffers
std::vector<Framebuffer*> fullscreen_framebuffers; 
Framebuffer* map_framebuffer_ptr; // This framebuffer will be used to display an birds-eye view of the world centered on the player

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void setup_input_keys();

// input related parameters
float cursor_x, cursor_y;
bool firstMouse = true;
bool capture_mouse = true;

// Random
utils::random::generator rng;

// Physics
PhysicsEngine<Entity> physics_engine;
bool phy_debug_mode = false;
float maxSecPerFrame = 1.0f / 60.0f;
float capped_deltaTime;

// Scene
std::function<void()> scene_setup;
Player player;
bool hold_to_fire = true;

// parameters for time computation
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

// boolean to activate/deactivate wireframe rendering
bool wireframe = false;

// Lights
PointLight* currentLight;
float mov_light_speed = 5.f;

// Shaders
float paintblur_shader_blurstrength = 10.f;

// Others
unsigned int map_framebuffer_divisor = 4;

/////////////////// INPUT functions ///////////////////////

// We use these button callbacks to simply relay the key information to our input handler
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		Input::instance().press_key(button);
	}
	else if (action == GLFW_RELEASE)
	{
		Input::instance().release_key(button);
	}
}

// Here we setup our input handler, assigning key events to an action (a lambda function)
void setup_input_keys()
{
	// Pressed input (active while being pressed)
	{
		// Camera movement
		Input::instance().add_onPressed_callback(GLFW_KEY_W, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::FORWARD, capped_deltaTime); });
		Input::instance().add_onPressed_callback(GLFW_KEY_S, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::BACKWARD, capped_deltaTime); });
		Input::instance().add_onPressed_callback(GLFW_KEY_A, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::LEFT, capped_deltaTime); });
		Input::instance().add_onPressed_callback(GLFW_KEY_D, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::RIGHT, capped_deltaTime); });
		Input::instance().add_onPressed_callback(GLFW_KEY_Q, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::DOWN, capped_deltaTime); });
		Input::instance().add_onPressed_callback(GLFW_KEY_E, [&]() { player.first_person_camera.ProcessKeyboard(Camera::Directions::UP, capped_deltaTime); });

		// Simple light movement
		Input::instance().add_onPressed_callback(GLFW_KEY_LEFT , [&]() { currentLight->position.x -= mov_light_speed * capped_deltaTime; });
		Input::instance().add_onPressed_callback(GLFW_KEY_RIGHT, [&]() { currentLight->position.x += mov_light_speed * capped_deltaTime; });
		Input::instance().add_onPressed_callback(GLFW_KEY_UP   , [&]() { currentLight->position.z -= mov_light_speed * capped_deltaTime; });
		Input::instance().add_onPressed_callback(GLFW_KEY_DOWN , [&]() { currentLight->position.z += mov_light_speed * capped_deltaTime; });

		// Shoot button (hold version)
		Input::instance().add_onPressed_callback(GLFW_MOUSE_BUTTON_RIGHT, [&]()
			{
				// This control is a workaround to avoid triggering both onRelease and onPressed callbacks at the same time
				if (!hold_to_fire) return;

				player.shoot();
			});
	}

	// Toggled input (active while button released)
	{
		Input::instance().add_onRelease_callback(GLFW_KEY_ESCAPE, [&]() { wdw_ptr->close(); }); // Exit application

		// Toggle capture mouse (useful between alternating UI usage and gameplay)
		Input::instance().add_onRelease_callback(GLFW_KEY_LEFT_ALT, [&]() { capture_mouse = !capture_mouse; });

		// Remove camera vertical constraints
		Input::instance().add_onRelease_callback(GLFW_KEY_SPACE, [&]() { player.first_person_camera.toggle_fly(); }); 
		
		// Reset scene transforms to starting positions
		Input::instance().add_onRelease_callback(GLFW_KEY_R, [&]() { scene_setup(); });

		// Draw wireframe or filled
		Input::instance().add_onRelease_callback(GLFW_KEY_L, [&]()
			{
				wireframe = !wireframe;
				if (wireframe)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);// Draw in wireframe
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);// Fill in fragments
			});

		// Debug draw physics wireframe or filled
		Input::instance().add_onRelease_callback(GLFW_KEY_P, [&]()
			{
				phy_debug_mode = !phy_debug_mode;
				physics_engine.set_debug_mode(phy_debug_mode);
			});

		// Shoot button (press version)
		Input::instance().add_onRelease_callback(GLFW_MOUSE_BUTTON_RIGHT, [&]()
			{
				// This control is a workaround to avoid triggering both onRelease and onPressed callbacks at the same time
				if (hold_to_fire) return;

				player.shoot();
			});
	}
}

// Here we relay the information about mouse movement to the camera
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
		player.first_person_camera.ProcessMouseMovement(x_offset, y_offset);
}

// Framebuffer resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	ws.width = width;
	ws.height = height;
	
	// Resize all screensized framebuffers
	for (auto f : fullscreen_framebuffers)
	{
		f->resize(width, height);
	}
	map_framebuffer_ptr->resize(width / map_framebuffer_divisor, height / map_framebuffer_divisor);

	// Paintballs blur depends on framebuffer size so we need to adjust blur strength on resize
	paintblur_shader_blurstrength = std::powf(static_cast<float>(ws.width * ws.height), 0.16f);
}


/////////////////// MAIN function ///////////////////////
int main()
{
#pragma region window_setup
	Window wdw
	{
		Window::window_create_info
		{
			{ "Chromancers: splatting colors!" }, //.title
			{ 4 }, //.gl_version_major
			{ 6 }, //.gl_version_minor
			{ 1280 }, //.window_width
			{ 720 }, //.window_height
			{ 1280 }, //.viewport_width
			{ 720 }, //.viewport_height
			{ true }, //.resizable
			{ false }, //.vsync
			{ true }, //.debug_gl
		}
	};

	wdw_ptr = &wdw;

	GLFWwindow* glfw_window = wdw.get();
	ws = wdw.get_size();

	// Callbacks linking with glfw
	glfwSetKeyCallback(glfw_window, key_callback);
	glfwSetCursorPosCallback(glfw_window, mouse_pos_callback);
	glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
	glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);

	// Setup keys
	setup_input_keys();

	// Imgui setup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	ImPlot::CreateContext();
#pragma endregion window_setup

#pragma region scene_setup	
	// Camera setup
	Camera topdown_camera;

	// Scene setup
	Scene main_scene{ rng };
	main_scene.current_camera = &player.first_person_camera;
#pragma endregion scene_setup

#pragma region shader_setup
	// Shader and lights setup

	// Utils shaders are common types, constants and functions that can be added on top of other compiled shaders
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	// Basic shaders for debugging purposes
	Shader basic_mvp_shader      { "basic_mvp_shader", "shaders/text/generic/mvp.vert", "shaders/text/generic/basic.frag", 4, 3 };
	Shader debug_shader          { "debug_shader", "shaders/text/generic/mvp.vert", "shaders/text/generic/fullcolor.frag", 4, 3 };

	// Lit shaders, will take into account point and directional lights for shading calculations as well as materials
	Shader default_lit_shader    { "default_lit", "shaders/text/default_lit.vert", "shaders/text/default_lit.frag", 4, 3, nullptr, utils_shaders };
	Shader default_lit_instanced { "default_lit_instanced", "shaders/text/default_lit_instanced.vert", "shaders/text/default_lit.frag", 4, 3, nullptr, utils_shaders };

	// Simple shader that applies a texture to a volume (especially used for full-screen quads)
	Shader textured_shader       { "textured_shader", "shaders/text/generic/textured.vert" , "shaders/text/generic/textured.frag", 4, 3 };

	// Shader for paintable objects, will load/store color values onto a paintmap
	Shader painter_shader        { "painter_shader", "shaders/text/generic/texpainter.vert" , "shaders/text/generic/texpainter.frag", 4, 3 };

	// Shaders for shadowmap calculation, respectively for directional lights and for point lights
	Shader shadowmap_shader      { "shadowmap_shader", "shaders/text/generic/shadow_map.vert" , "shaders/text/generic/shadow_map.frag", 4, 3 };
	Shader shadowcube_shader     { "shadowcube_shader", "shaders/text/generic/shadow_cube.vert" , "shaders/text/generic/shadow_cube.frag", 4, 3, "shaders/text/generic/shadow_cube.geom" };

	// Shaders for post-processing effects on the paintballs, to make them look more organic instead of single particles
	Shader paintblur_shader      { "paintblur_shader", "shaders/text/generic/postprocess.vert" , "shaders/text/generic/paintblur.frag", 4, 3 };
	Shader paintstep_shader      { "paintstep_shader", "shaders/text/generic/postprocess.vert" , "shaders/text/generic/paintstep.frag", 4, 3 };
	
	// Shader for merging two framebuffers into one, using their color and depth buffers as textures to determine the resulting fragments
	Shader mergefbo_shader       { "mergefbo_shader", "shaders/text/generic/postprocess.vert" , "shaders/text/generic/merge_fbo.frag", 4, 3 };

	// Bundling up shaders in collections to later perform shader uniform setting operations in bulk
	std::vector <std::reference_wrapper<Shader>> lit_shaders, all_shaders;
	lit_shaders.push_back(default_lit_shader); lit_shaders.push_back(default_lit_instanced); 
	all_shaders.push_back(basic_mvp_shader); all_shaders.push_back(default_lit_shader); all_shaders.push_back(default_lit_instanced); 

	// Lights and shadowmaps setup 
	std::vector<unsigned int> shadowmap_res{128, 256, 512, 1024, 2048, 4096};
	unsigned int current_dl_res = shadowmap_res[4];
	unsigned int current_pl_res = shadowmap_res[2];

	DirectionalLight::ShadowMapSettings dir_sm_settings;
	dir_sm_settings.resolution = current_dl_res;
	dir_sm_settings.shader = &shadowmap_shader;

	PointLight::ShadowMapSettings pl_sm_settings;
	pl_sm_settings.resolution = current_pl_res;
	pl_sm_settings.shader = &shadowcube_shader;

	PointLight pl1 { glm::vec3{-8.0f, 2.0f, 2.5f}, glm::vec4{1, 0, 1, 1}, 0.5f, pl_sm_settings};
	PointLight pl2 { glm::vec3{-8.0f, 2.0f, 7.5f}, glm::vec4{0, 1, 1, 1}, 0.5f, pl_sm_settings};
	PointLight pl3 { glm::vec3{-8.0f, 2.0f, 7.5f}, glm::vec4{0, 1, 1, 1}, 0.5f, pl_sm_settings};
	DirectionalLight dl1{ glm::vec3{ -1, -1, -1 }, glm::vec4{1, 1, 1, 1}, 1.0f, dir_sm_settings};
	DirectionalLight dl2{ glm::vec3{ -1, -1,  0 }, glm::vec4{1, 1, 1, 1}, 0.5f, dir_sm_settings};
	DirectionalLight dl3{ glm::vec3{  1, -1,  0 }, glm::vec4{1, 1, 1, 1}, 0.5f, dir_sm_settings};

	std::vector<PointLight*> point_lights;
	point_lights.push_back(&pl1); point_lights.push_back(&pl2); //point_lights.push_back(&pl3);

	std::vector<DirectionalLight*> dir_lights;
	dir_lights.push_back(&dl1); dir_lights.push_back(&dl2); //dir_lights.push_back(&dl3);

	if(point_lights.size() > 0) currentLight = point_lights[0];

	// Shader uniform commons and constants setup
	for (Shader& shader : all_shaders)
	{
		shader.bind();
		shader.setMat4("projectionMatrix", main_scene.current_camera->projectionMatrix());
		shader.setVec3("wCameraPos", main_scene.current_camera->position());
	}
	for (Shader& lit_shader : lit_shaders)
	{
		lit_shader.bind();
		lit_shader.setUint("nPointLights", gsl::narrow<unsigned int>(point_lights.size()));
		lit_shader.setUint("nDirLights", gsl::narrow<unsigned int>(dir_lights.size()));
	}

	// Specific uniforms setup (we use these in imgui for interactability)
	float paintstep_shader_t0_color = 0.1f, paintstep_shader_t1_color = 0.4f;
	float paintstep_shader_t0_alpha = 0.8f, paintstep_shader_t1_alpha = 0.9f;
	float paintblur_shader_depthoffset = 1.f;
	bool paintblur_shader_ignore_alpha = false;
	int paintblur_blurpasses = 4;

#pragma endregion shader_setup

#pragma region materials_setup
	// Textures setup
	Texture soilcracked_diffuse_tex{ "textures/soil_cracked.png" }, soilcracked_normal_tex{ "textures/soil_cracked_normal.png" }, soilcracked_depth_tex{ "textures/soil_cracked_disp.png" };
	Texture greybricks_diffuse_tex{ "textures/brickwall.png" }, greybricks_normal_tex{ "textures/brickwall_normal.png" }, greybricks_depth_tex{ "textures/brickwall_disp.png" };
	Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" },
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex{ "textures/bricks2_disp.jpg" };
	Texture splat_tex{ "textures/uniform_splat_inv.png" }, splat_normal_tex{"textures/splat_normal.jpg"};

	// Shared materials
	Material default_lit_material{ default_lit_shader };

	Material redbricks_mat{ default_lit_material };
	redbricks_mat.diffuse_map = &redbricks_diffuse_tex; redbricks_mat.normal_map = &redbricks_normal_tex; redbricks_mat.displacement_map = &redbricks_depth_tex;
	redbricks_mat.parallax_heightscale = 0.05f;

	Material grey_bricks{ default_lit_material };
	grey_bricks.diffuse_map = &greybricks_diffuse_tex; grey_bricks.normal_map = &greybricks_normal_tex; grey_bricks.displacement_map = &greybricks_depth_tex;
	grey_bricks.parallax_heightscale = 0.01f;

	Material soil_cracked{ default_lit_material };
	soil_cracked.diffuse_map = &soilcracked_diffuse_tex; soil_cracked.normal_map = &soilcracked_normal_tex; soil_cracked.displacement_map = &soilcracked_depth_tex;
	soil_cracked.parallax_heightscale = 0.001f;

	Material sph_mat { default_lit_material };

	Material basic_mat{ basic_mvp_shader };

	// Specific materials (ad hoc)
	Material left_room_wall_material { redbricks_mat };
	left_room_wall_material.uv_repeat = 20.f; left_room_wall_material.detail_normal_bias = 0.25f;

	Material left_room_lwall_material { left_room_wall_material };
	Material left_room_rwall_material { left_room_wall_material };
	Material left_room_bwall_material { left_room_wall_material };
	
	Material front_wall_material { grey_bricks };
	front_wall_material.uv_repeat = 20.f; 

	Material floor_material{ soil_cracked }; 
	floor_material.uv_repeat = 0.5f * std::max(floor_material.diffuse_map->width(), floor_material.diffuse_map->height());
	floor_material.detail_normal_bias = 0.05f;

	Material cube_material { redbricks_mat };
	cube_material.uv_repeat = 3.f;

	Material test_cube_material{ default_lit_material };
	Material fountain_material{ default_lit_material };
	Material gun_mat { default_lit_material };
	Material buny_mat{ default_lit_material };
	buny_mat.uv_repeat = 20.f;

#pragma endregion materials_setup

#pragma region entities_setup
	// Model loading and scene entities emplacement
	Model cube_model{ "models/cube.obj" }, sphere_model{ "models/sphere.obj" }, bunny_model{ "models/bunny.obj" };
	Model gun_model{ "models/gun/gun.obj" }; Model paintball_model{ "models/drop_lowres.obj" }; 
	Model quad_mesh{ Mesh::simple_quad_mesh() };

	Entity* cube        = main_scene.emplace_entity("cube", "brick_cube", cube_model, cube_material);
	Entity* test_cube   = main_scene.emplace_entity("test_cube", "test_cube", cube_model, test_cube_material);
	Entity* floor_plane = main_scene.emplace_entity("floor", "floorplane", cube_model, floor_material);
	Entity* wall_plane  = main_scene.emplace_entity("wall", "wallplane", cube_model, front_wall_material);
	Entity* sphere      = main_scene.emplace_entity("sphere", "sphere", sphere_model, sph_mat);

	Entity* left_room_lwall = main_scene.emplace_entity("wall", "left_room_leftwall", cube_model, left_room_lwall_material);
	Entity* left_room_rwall = main_scene.emplace_entity("wall", "left_room_rightwall", cube_model, left_room_rwall_material);
	Entity* left_room_bwall = main_scene.emplace_entity("wall", "left_room_backwall", cube_model, left_room_bwall_material);
	Entity* bunny           = main_scene.emplace_entity("bunny", "buny", bunny_model, buny_mat);	
	Entity* fountain        = main_scene.emplace_entity("fountain", "Water fountain", cube_model, fountain_material);
	Entity* fountain_bunny1 = main_scene.emplace_entity("bunny fountain", "Bunny left fountain", cube_model, fountain_material);
	Entity* fountain_bunny2 = main_scene.emplace_entity("bunny fountain", "Bunny right fountain", cube_model, fountain_material);
	Entity* hor_stream      = main_scene.emplace_entity("hor_stream", "Horizontal stream", cube_model, fountain_material);

	// Map cursor setup
	Model triangle_mesh{ Mesh::simple_triangle_mesh() };
	Entity cursor{ "cursor", triangle_mesh, basic_mat };

	// Lightcubes setup
	std::vector<Material> lightcube_materials; lightcube_materials.resize(point_lights.size());
	std::vector<Entity*> lightcube_entites; lightcube_entites.resize(point_lights.size());
	for (size_t i = 0; i < point_lights.size(); i++)
	{
		Material lightcolor { default_lit_material };
		lightcolor.ambient_color = point_lights[i]->color;
		lightcolor.kA = 1; lightcolor.kD = 0; lightcolor.kS = 0;
		lightcolor.receive_shadows = false;
		lightcube_materials[i] = lightcolor;

		lightcube_entites[i] = main_scene.emplace_entity("light_cube", "light_cube", cube_model, lightcube_materials[i]);
	}

	// Scene entities setup
	// We are setting this as a lambda so we can reset every transform to this initial configuration even at runtime
	scene_setup = [&]()
	{
		floor_plane->set_size(glm::vec3(25.0f, 0.2f, 25.0f));
		floor_plane->set_position(glm::vec3(0.0f, -1.0f, 0.0f));

		wall_plane->set_size(glm::vec3(10.0f, 5.0f, 0.1f));
		wall_plane->set_orientation(glm::vec3(-30.0f, 0.0f, 0.f));
		wall_plane->set_position(glm::vec3(0.0f, 3.0f, -12.0f));
		
		left_room_lwall->set_size(glm::vec3(5.0f, 5.0f, 0.1f));
		left_room_lwall->set_position(glm::vec3(-10.0f, 4.0f, 10.0f));
		
		left_room_rwall->set_size(glm::vec3(5.0f, 5.0f, 0.1f));
		left_room_rwall->set_position(glm::vec3(-10.0f, 4.0f, 0.0f));

		left_room_bwall->set_size(glm::vec3(5.0f, 5.0f, 0.1f));
		left_room_bwall->set_orientation(glm::vec3(0.0f, 90.0f, 0.f));
		left_room_bwall->set_position(glm::vec3(-15.0f, 4.0f, 5.f));

		cube->set_position(glm::vec3(0.0f, 3.0f, 0.0f));

		test_cube->set_position(glm::vec3(-12.0f, 0.0f, 5.f));

		sphere->set_size(glm::vec3(0.25f));

		bunny->set_size(glm::vec3(.75f));
		bunny->set_position(glm::vec3(5.0f, 3.0f, -2.0f));

		fountain->set_size(glm::vec3(0.1f));
		fountain->set_orientation(glm::vec3(90.f, 0.0f, 0.0f));
		fountain->set_position(glm::vec3(-12.0f, 2.0f, 5.f));
		
		fountain_bunny1->set_size(glm::vec3(0.1f));
		fountain_bunny1->set_orientation(glm::vec3(0.0f, -90.0f, 45.0f));
		fountain_bunny1->set_position(glm::vec3(0.0f, 10.0f, -2.f));

		fountain_bunny2->set_size(glm::vec3(0.1f));
		fountain_bunny2->set_orientation(glm::vec3(0.0f, 90.0f, 45.0f));
		fountain_bunny2->set_position(glm::vec3(9.0f, 10.0f, -2.f));

		hor_stream->set_size(glm::vec3(0.1f));
		hor_stream->set_orientation(glm::vec3(0.0f, 180.0f, 0.0f));
		hor_stream->set_position(glm::vec3(9.0f, 0.0f, 20.f));

		for (auto& lightcube_entity : lightcube_entites)
		{
			lightcube_entity->set_size(glm::vec3(0.25f));
		}

		cursor.set_size(glm::vec3(3.0f));
	};
	scene_setup(); // We call the lambda for the first time to setup the scene

	// Player setup
	Entity gun{"gun", gun_model, gun_mat}; // we don't treat player entities like other objects on the scene because we have a different update/drawing logic

	PaintballSpawner gun_spawner{ physics_engine, rng, default_lit_instanced };
	gun_spawner.paintball_model = &paintball_model;
	gun_spawner.current_scene = &main_scene;

	gun_spawner.paintball_material.shininess = 64.f; 
	gun_spawner.paintball_material.kA = 0.17f; gun_spawner.paintball_material.kD = 0.17f; gun_spawner.paintball_material.kS = 0.5f; 
	gun_spawner.paintball_material.receive_shadows = false;
	
	player.paintball_spawner = &gun_spawner;

	player.viewmodel_offset = {0.3,-0.2,0.25};
	player.gun_entity = &gun;

	// Physics setup
	GLDebugDrawer phy_debug_drawer{ *main_scene.current_camera, debug_shader };
	physics_engine.addDebugDrawer(&phy_debug_drawer);
	physics_engine.set_debug_mode(phy_debug_mode);

	// Physical entities setup (an entity that has a collider(thus a rigidbody) and can be influenced by forces)
	floor_plane->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	wall_plane ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	test_cube  ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 0.1f, 0.1f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	cube       ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 30.f, 0.1f, 0.1f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	sphere     ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 1.0f, 1.0f, {ColliderShape::SPHERE, glm::vec3{1}} }, true);

	left_room_lwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,  glm::vec3{1}} }, true);
	left_room_rwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,  glm::vec3{1}} }, true);
	left_room_bwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,  glm::vec3{1}} }, true);

	std::vector<glm::vec3> bunny_mesh_vertices = bunny_model.get_vertices_positions();
	bunny          ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1000.0f, 1.0f, 1.0f,
		ColliderShapeCreateInfo{ ColliderShape::HULL, bunny->world_transform().size(), &bunny_mesh_vertices}}, false);

	// Paintable entities setup (an entity that has a paintmap, thus its appearance can be changed by paintballs)
	test_cube      ->emplace_component<PaintableComponent>(painter_shader, 128, 128, &splat_tex, &splat_normal_tex);
	wall_plane     ->emplace_component<PaintableComponent>(painter_shader, 512, 512, &splat_tex, &splat_normal_tex);
	cube           ->emplace_component<PaintableComponent>(painter_shader, 128, 128, &splat_tex, &splat_normal_tex);
	floor_plane    ->emplace_component<PaintableComponent>(painter_shader, 2048, 2048, &splat_tex, &splat_normal_tex);
	bunny          ->emplace_component<PaintableComponent>(painter_shader, 256, 256, &splat_tex, &splat_normal_tex);

	left_room_lwall->emplace_component<PaintableComponent>(painter_shader, 512, 512, &splat_tex, &splat_normal_tex);
	left_room_rwall->emplace_component<PaintableComponent>(painter_shader, 512, 512, &splat_tex, &splat_normal_tex);
	left_room_bwall->emplace_component<PaintableComponent>(painter_shader, 512, 512, &splat_tex, &splat_normal_tex);

	// Paintball spawners setup (an entity that generates paintballs)
	PaintballSpawnerComponent* fountain_spawner = static_cast<PaintballSpawnerComponent*> 
		(fountain   -> emplace_component<PaintballSpawnerComponent>(physics_engine, rng, paintball_model, default_lit_instanced));
	PaintballSpawnerComponent* fountain_bunny_spawner_sx = static_cast<PaintballSpawnerComponent*> 
		(fountain_bunny1 -> emplace_component<PaintballSpawnerComponent>(physics_engine, rng, paintball_model, default_lit_instanced));
	PaintballSpawnerComponent* fountain_bunny_spawner_dx = static_cast<PaintballSpawnerComponent*> 
		(fountain_bunny2 -> emplace_component<PaintballSpawnerComponent>(physics_engine, rng, paintball_model, default_lit_instanced));

	PaintballSpawnerComponent* hor_spawner = static_cast<PaintballSpawnerComponent*> 
		(hor_stream -> emplace_component<PaintballSpawnerComponent>(physics_engine, rng, paintball_model, default_lit_instanced));

	std::vector<PaintballSpawnerComponent*> fountain_spawners; 
	fountain_spawners.push_back(fountain_spawner); 
	fountain_spawners.push_back(fountain_bunny_spawner_sx);  fountain_spawners.push_back(fountain_bunny_spawner_dx);
	fountain_spawners.push_back(hor_spawner);

	// shared spawner settings
	for (auto& f : fountain_spawners)
	{
		f->paintball_spawner.shooting_speed = 8.f; f->paintball_spawner.shooting_spread = 1.f;
		f->paintball_spawner.rounds_per_second = 100;
		f->paintball_spawner.size_variation_min_multiplier = 0.75f;
		f->paintball_spawner.size_variation_max_multiplier = 1.25f;
		f->paintball_spawner.paint_color = { 0.1f, 0.64f, 0.92f, 1.f };

		Material& fountain_ball_material = f->paintball_spawner.paintball_material;
		fountain_ball_material.shininess = 64.f; 
		fountain_ball_material.kA = 0.17f; fountain_ball_material.kD = 0.17f; fountain_ball_material.kS = 0.5f; 
		fountain_ball_material.receive_shadows = false;
	}

	// specific spawner settings
	fountain_bunny_spawner_sx->paintball_spawner.paint_color = { 0.1f, 0.5f, 0.f, 1.f };
	fountain_bunny_spawner_sx->paintball_spawner.shooting_spread = 2.f;
	fountain_bunny_spawner_sx->paintball_spawner.shooting_speed = 6.f;
	fountain_bunny_spawner_dx->paintball_spawner.paint_color = { 0.5f, 0.f, 0.5f, 1.f };
	fountain_bunny_spawner_dx->paintball_spawner.shooting_spread = 2.f;
	fountain_bunny_spawner_dx->paintball_spawner.shooting_speed = 6.f;

	hor_spawner->paintball_spawner.paint_color = { 1.f, 0.85f, 0.f, 1.f };
	
#pragma endregion entities_setup

#pragma region framebuffers_setup
	// Framebuffers setup
	// This framebuffer will be used to display the entities from the whole scene except the paintballs
	Framebuffer world_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };
	
	// This framebuffer will be used to display the instanced paintball entities
	Framebuffer paintballs_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };
	
	// These framebuffers will be used for the "ping-pong" blur passes
	Framebuffer postfxblur0_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };
	Framebuffer postfxblur1_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };
	
	// This framebuffer will be used for the smoothstep effect of paintballs
	Framebuffer paintstep_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };

	Framebuffer map_framebuffer{ ws.width / map_framebuffer_divisor, ws.height / map_framebuffer_divisor, 
		Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };

	// This framebuffer will be used to group
	Framebuffer present_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };

	fullscreen_framebuffers = {&world_framebuffer, & paintballs_framebuffer,
		& postfxblur0_framebuffer, & postfxblur1_framebuffer, & paintstep_framebuffer, & present_framebuffer};

	std::vector<Framebuffer*> postfx_fbos; postfx_fbos.push_back(&postfxblur0_framebuffer); postfx_fbos.push_back(&postfxblur1_framebuffer);

	map_framebuffer_ptr = &map_framebuffer;
#pragma endregion framebuffers_setup
	
#pragma region pre-loop_setup
	main_scene.init();
	player.init();

	// Fps Measurements variables setup
	const int fps_values_amount = 2000;
	utils::containers::FixedQueue<float, fps_values_amount> fps_values;
	utils::containers::FixedQueue<float, fps_values_amount> t_values;

	float avg_ms_per_frame = 1.f, avg_fps = 1.f, alpha = 0.9f;
	float min_fps = 0.f, max_fps = 800.f;
	float time_offset = 3.0f, fps_offset = 100.f;
	bool stable = false;
#pragma endregion pre-loop_setup

#pragma region rendering_loop
	// Rendering loop
	while (wdw.is_open())
	{
#pragma region setup_loop
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		float currentFrameTime = gsl::narrow_cast<float>(glfwGetTime());
		deltaTime = currentFrameTime - lastFrameTime;
		capped_deltaTime = deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame; // for physics
		lastFrameTime = currentFrameTime;

		if (!stable && currentFrameTime > 5)
		{
			stable = true;
			min_fps = 1000.f, max_fps = 0.f;
		}

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

		// Render objects with a window-wide viewport
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height);

		// Update commons for all shaders 
		for (Shader& shader : all_shaders)
		{
			shader.bind();
			shader.setVec3("wCameraPos", main_scene.current_camera->position());
			shader.setMat4("viewMatrix", main_scene.current_camera->viewMatrix());
		}

		// Update lighting for lit shaders
		for (Shader& lit_shader : lit_shaders)
		{
			lit_shader.bind();
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				point_lights[i]->setup(lit_shader, i);
			}
			for (size_t i = 0; i < dir_lights.size(); i++)
			{
				dir_lights[i]->setup(lit_shader, i);
			}
			lit_shader.unbind();
		}

		// Update lightcubes
		for (size_t i = 0; i < point_lights.size(); i++)
		{
			lightcube_entites[i]->set_position(point_lights[i]->position);
			lightcube_entites[i]->material->ambient_color = point_lights[i]->color;
		}

#pragma endregion setup_loop

#pragma region update_world
		// Update physics simulation
		physics_engine.step(capped_deltaTime);
		physics_engine.detect_collisions();

		// Update entities
		main_scene.update(capped_deltaTime);
		player.update(capped_deltaTime);

#pragma endregion update_world

#pragma region shadow_pass

		for (auto& light : dir_lights)
		{
			light->compute_shadowmap(main_scene);
		}
		for (auto& light : point_lights)
		{
			light->compute_shadowmap(main_scene);
		}

		// Update lit shaders to add the computed shadowmap(s)
		std::array<GLint, MAX_DIR_LIGHTS> dir_shadow_locs;
		std::array<GLint, MAX_POINT_LIGHTS> point_shadow_locs;
		int dir_shadow_locs_amount = gsl::narrow<int>(dir_shadow_locs.size());
		int point_shadow_locs_amount = gsl::narrow<int>(point_shadow_locs.size());

		for (Shader& lit_shader : lit_shaders)
		{
			lit_shader.bind();
			{
				// Set sampler locations for directional shadow maps
				for (int i = 0; i < dir_shadow_locs_amount; i++)
				{
					int tex_offset = SHADOW_TEX_UNIT + i;
					dir_shadow_locs[i] = tex_offset;
					glActiveTexture(GL_TEXTURE0 + tex_offset);
					glBindTexture(GL_TEXTURE_2D, i < dir_lights.size() ? dir_lights[i]->depthmap : 0);
				}
				// Set sampler locations for point shadow maps
				for (int i = 0; i < point_shadow_locs_amount; i++)
				{
					int tex_offset = SHADOW_TEX_UNIT + dir_shadow_locs_amount + i;
					point_shadow_locs[i] = tex_offset;
					glActiveTexture(GL_TEXTURE0 + tex_offset);
					glBindTexture(GL_TEXTURE_CUBE_MAP, i < point_lights.size() ? point_lights[i]->depthCubemap : 0);
				}

				lit_shader.setIntV("directional_shadow_maps", dir_shadow_locs_amount, dir_shadow_locs.data());
				lit_shader.setIntV("point_shadow_maps", point_shadow_locs_amount, point_shadow_locs.data());
			}
			lit_shader.unbind();
		}

#pragma endregion shadow_pass

#pragma region draw_world
		// Render scene
		world_framebuffer.bind();
		{
			glClearColor(0.26f, 0.46f, 0.98f, 1.0f); // bluish
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			main_scene.draw_except_instanced();
		}
		world_framebuffer.unbind();

		// Draw paintballs in their own framebuffer
		paintballs_framebuffer.bind();
		{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			main_scene.draw_only_instanced();
		}
		paintballs_framebuffer.unbind();

		// We perform a number of blur passes to make paintball countours less evident, rendering the whole paintball stream more cohesive
		Framebuffer *destination_fb = postfx_fbos[0], *source_fb = &paintballs_framebuffer, *paintblur_fb = &paintballs_framebuffer;
		paintblur_shader.bind();
		{
			for (int i = 0; i < paintblur_blurpasses; i++)
			{
				bool horizontal = i % 2;

				destination_fb->bind();
				{
					glClearColor(0, 0, 0, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glActiveTexture(GL_TEXTURE0);
					source_fb->get_color_attachment().bind();
					paintblur_shader.setInt("image", 0);

					glActiveTexture(GL_TEXTURE1);
					paintballs_framebuffer.get_depth_attachment().bind();
					paintblur_shader.setInt("depth_image", 1);
					
					paintblur_shader.setFloat("near_plane", main_scene.current_camera->near_plane());
					paintblur_shader.setFloat("far_plane", main_scene.current_camera->far_plane());

					paintblur_shader.setBool("horizontal", horizontal);
					paintblur_shader.setFloat("blur_strength", paintblur_shader_blurstrength);
					paintblur_shader.setFloat("depth_offset", paintblur_shader_depthoffset);
					paintblur_shader.setBool("ignore_alpha", paintblur_shader_ignore_alpha);
					quad_mesh.draw();
				}
				destination_fb->unbind();

				// Swap source and destination buffers
				destination_fb = postfx_fbos[!horizontal];
				source_fb      = postfx_fbos[ horizontal];
				paintblur_fb   = source_fb;
			}
		}
		paintblur_shader.unbind();

		// We play with color and alpha levels of the blurred paintballs to make them look more organic and liquidy
		paintstep_framebuffer.bind();
		{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			paintstep_shader.bind();
			{
				glActiveTexture(GL_TEXTURE0);
				paintblur_fb->get_color_attachment().bind(); // We take the last destination framebuffer
				paintstep_shader.setInt("image", 0);
				paintstep_shader.setFloat("t0_color", paintstep_shader_t0_color);
				paintstep_shader.setFloat("t1_color", paintstep_shader_t1_color);
				paintstep_shader.setFloat("t0_alpha", paintstep_shader_t0_alpha);
				paintstep_shader.setFloat("t1_alpha", paintstep_shader_t1_alpha);
				quad_mesh.draw();
			}
			paintstep_shader.unbind();
		}
		paintstep_framebuffer.unbind();

		// Merge world and post-processed paintball framebuffers
		present_framebuffer.bind();
		{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			mergefbo_shader.bind();
			{
				glActiveTexture(GL_TEXTURE0);
				world_framebuffer.get_color_attachment().bind();
				mergefbo_shader.setInt("fbo0_color", 0);

				glActiveTexture(GL_TEXTURE1);
				world_framebuffer.get_depth_attachment().bind();
				mergefbo_shader.setInt("fbo0_depth", 1);

				glActiveTexture(GL_TEXTURE2);
				paintstep_framebuffer.get_color_attachment().bind();
				mergefbo_shader.setInt("fbo1_color", 2);

				glActiveTexture(GL_TEXTURE3);
				paintballs_framebuffer.get_depth_attachment().bind(); // N.B. we use the paintballs depth buffer because we lost the depth info while processing it!!
				mergefbo_shader.setInt("fbo1_depth", 3);

				quad_mesh.draw();
			}
			mergefbo_shader.unbind();

			// Draw gun on top of world
			glClear(GL_DEPTH_BUFFER_BIT);
			player.draw();
		}
		present_framebuffer.unbind();

#pragma endregion draw_world

#pragma region map_draw

		// MAP
		bool cull_option = main_scene.use_frustum_culling;
		map_framebuffer.bind();
		{
			glClearColor(0.26f, 0.46f, 0.98f, 1.0f); // bluish
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
			float topdown_height = 20;
			topdown_camera.set_position(player.first_person_camera.position() + glm::vec3{ 0, topdown_height, 0 });
			topdown_camera.lookAt(player.first_person_camera.position(), glm::vec3{0, 0, 1});
			main_scene.current_camera = &topdown_camera;
			main_scene.use_frustum_culling = false;
		
			// Update camera info since we swapped to topdown
			for (Shader& shader : all_shaders)
			{
				shader.bind();
				shader.setVec3("wCameraPos", main_scene.current_camera->position());
				shader.setMat4("viewMatrix", main_scene.current_camera->viewMatrix());
			}
		
			// Redraw all scene objects from map pov except paintballs
			main_scene.draw_except_instanced();
		
			// Prepare cursor shader
			glClear(GL_DEPTH_BUFFER_BIT);
		
			cursor.set_position(player.first_person_camera.position());
			cursor.set_orientation({ -90.0f, 0.0f, -player.first_person_camera.rotation().y - 90.f });
		
			cursor.update(capped_deltaTime);
			cursor.draw();
		}
		map_framebuffer.unbind();

		// Reset main scene camera to the player's one
		main_scene.current_camera = &player.first_person_camera;
		main_scene.use_frustum_culling = cull_option; // restore frustum culling option

#pragma endregion map_draw

#pragma region present
		// World present onto default framebuffer
		glBlitNamedFramebuffer(present_framebuffer.id(), 0, 0, 0, ws.width, ws.height, 0, 0, ws.width, ws.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// Draw physics colliders on top if debug mode is enabled
		physics_engine.debug_draw_world();

		// Map present : we render now into the smaller map viewport
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(ws.width - map_framebuffer.width() - 10, ws.height - map_framebuffer.height() - 10, map_framebuffer.width(), map_framebuffer.height());
		textured_shader.bind();
		{
			glActiveTexture(GL_TEXTURE0);
			map_framebuffer.get_color_attachment(0).bind();
			quad_mesh.draw();
			textured_shader.unbind();
		}
		glViewport(0, 0, ws.width, ws.height);

#pragma endregion present

#pragma region imgui_draw
		// ImGUI window creation
		ImGui_ImplOpenGL3_NewFrame();// Tell OpenGL a new Imgui frame is about to begin
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("FPS info");
		{
			avg_ms_per_frame  = alpha * avg_ms_per_frame + (1.0f - alpha) * (deltaTime * 1000);
			avg_fps = alpha * avg_fps + (1.0f - alpha) * (1.f / deltaTime);
			if (stable)
			{
				min_fps = min_fps < avg_fps ? min_fps : avg_fps;
				max_fps = max_fps > avg_fps ? max_fps : avg_fps;
			}

			fps_values.push_back(avg_fps);
			t_values.push_back(currentFrameTime);
			
			std::string my_fps_counter = "Avg FPS: " + std::to_string(avg_fps);
			std::string ms_frame_counter = "Time per frame: " + std::to_string(avg_ms_per_frame) + "ms";
			std::string min_record = "Min fps:" + std::to_string(min_fps) + "ms";
			std::string max_record = "Max fps:" + std::to_string(max_fps) + "ms";
			std::string pballs_amount = "Current paintballs amount:" + std::to_string(main_scene.get_instances_amount()) + "ms";
			ImGui::Text(my_fps_counter.c_str()); ImGui::Text(ms_frame_counter.c_str());
			ImGui::Text(min_record.c_str()); ImGui::Text(max_record.c_str());
			ImGui::Text(pballs_amount.c_str());
			ImGui::SliderFloat("Time offset", &time_offset, 0, 10, " %.1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Fps offset", &fps_offset, 0, 200, " %.1f", ImGuiSliderFlags_AlwaysClamp);
			if (ImPlot::BeginPlot("##Fps Plot"))
			{
				ImPlot::SetupAxisLimits(ImAxis_X1, currentFrameTime - time_offset, currentFrameTime, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, min_fps - fps_offset, max_fps + fps_offset, ImPlotCond_Always);
				ImPlot::PlotLine("Fps", t_values.data(), fps_values.data(), fps_values_amount);
				ImPlot::EndPlot();
			}
		}
		ImGui::End();
		ImGui::Begin("Settings");
		{
			// Various settings
			if (ImGui::CollapsingHeader("Paintball FX settings"))
			{
				ImGui::ColorEdit4("Paint Color", glm::value_ptr(player.paintball_spawner->paint_color));
				ImGui::Separator(); ImGui::Text("Paintball material");
				ImGui::SliderFloat("kA", &player.paintball_spawner->paintball_material.kA, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("kD", &player.paintball_spawner->paintball_material.kD, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("kS", &player.paintball_spawner->paintball_material.kS, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::ColorEdit4("Ambient", glm::value_ptr(player.paintball_spawner->paintball_material.ambient_color));
				ImGui::ColorEdit4("Diffuse", glm::value_ptr(player.paintball_spawner->paintball_material.diffuse_color));
				ImGui::ColorEdit4("Specular", glm::value_ptr(player.paintball_spawner->paintball_material.specular_color));
				ImGui::SliderFloat("Shininess", &player.paintball_spawner->paintball_material.shininess, 0, 128.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::Checkbox("Receive shadows", &player.paintball_spawner->paintball_material.receive_shadows);
				ImGui::Separator(); ImGui::Text("Blur and step FX shaders");
				ImGui::SliderInt("Blur passes", &paintblur_blurpasses, 0, 8, "%d", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Blur Strength", &paintblur_shader_blurstrength, 0, 16.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Blur Depth offset", &paintblur_shader_depthoffset, 0, 16.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::Checkbox("Blur ignore alpha", &paintblur_shader_ignore_alpha);
				ImGui::SliderFloat("Smoothstep color t0", &paintstep_shader_t0_color, -1.f, 1.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Smoothstep color t1", &paintstep_shader_t1_color, -1.f, 1.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Smoothstep alpha t0", &paintstep_shader_t0_alpha, -1.f, 1.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Smoothstep alpha t1", &paintstep_shader_t1_alpha, -1.f, 1.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
			}
			if (ImGui::CollapsingHeader("Paintgun settings"))
			{
				ImGui::Separator(); ImGui::Text("Paintball");
				ImGui::SliderFloat("Weight", &player.paintball_spawner->paintball_weight, 0, 10, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Size", &player.paintball_spawner->paintball_size, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Min size variation", &player.paintball_spawner->size_variation_min_multiplier, 0, 2, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Max size variation", &player.paintball_spawner->size_variation_max_multiplier, 0, 2, " %.2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Separator(); ImGui::Text("Gun properties");
				ImGui::SliderFloat("Muzzle speed", &player.paintball_spawner->shooting_speed, 0, 100, " %.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Muzzle spread", &player.paintball_spawner->shooting_spread, 0, 3, " %.2f", ImGuiSliderFlags_AlwaysClamp);
				unsigned int rps_min = 0, rps_max = 1000;
				ImGui::SliderScalar("Rounds per Sec", ImGuiDataType_U32, &player.paintball_spawner->rounds_per_second, &rps_min, &rps_max, " %d", ImGuiSliderFlags_AlwaysClamp);
				ImGui::Checkbox("Hold to fire", &hold_to_fire);

				ImGui::Separator(); ImGui::Text("Gun model");
				ImGui::SliderFloat3("Viewmodel offset", glm::value_ptr(player.viewmodel_offset), -1, 1, "%.2f", 1);
			}

			// Paintball spawners
			if (ImGui::CollapsingHeader("Paintball spawners"))
			{
				ImGui::Indent();
				ImGui::PushID(&fountain_spawners);
				for (int i = 0; i < fountain_spawners.size(); i++)
				{
					auto& fountain_spawner = fountain_spawners[i];
					std::string fountain_label = fountain_spawner->parent()->display_name;

					if (ImGui::CollapsingHeader(fountain_label.c_str()))
					{
						ImGui::PushID(i);
						ImGui::Indent();
						ImGui::Separator(); ImGui::Text("Paintball material");
						ImGui::ColorEdit4("Paint Color", glm::value_ptr(fountain_spawner->paintball_spawner.paint_color));
						ImGui::SliderFloat("kA", &fountain_spawner->paintball_spawner.paintball_material.kA, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("kD", &fountain_spawner->paintball_spawner.paintball_material.kD, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("kS", &fountain_spawner->paintball_spawner.paintball_material.kS, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::ColorEdit4("Ambient", glm::value_ptr(fountain_spawner->paintball_spawner.paintball_material.ambient_color));
						ImGui::ColorEdit4("Diffuse", glm::value_ptr(fountain_spawner->paintball_spawner.paintball_material.diffuse_color));
						ImGui::ColorEdit4("Specular", glm::value_ptr(fountain_spawner->paintball_spawner.paintball_material.specular_color));
						ImGui::SliderFloat("Shininess", &fountain_spawner->paintball_spawner.paintball_material.shininess, 0, 128.f, " %.1f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::Checkbox("Receive shadows", &fountain_spawner->paintball_spawner.paintball_material.receive_shadows);

						ImGui::Separator(); ImGui::Text("Paintball");
						ImGui::SliderFloat("Weight", &fountain_spawner->paintball_spawner.paintball_weight, 0, 10, " %.1f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Size", &fountain_spawner->paintball_spawner.paintball_size, 0, 1, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Min size variation", &fountain_spawner->paintball_spawner.size_variation_min_multiplier, 0, 2, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Max size variation", &fountain_spawner->paintball_spawner.size_variation_max_multiplier, 0, 2, " %.2f", ImGuiSliderFlags_AlwaysClamp);

						ImGui::Separator(); ImGui::Text("Shoot properties");
						ImGui::SliderFloat("Shoot speed", &fountain_spawner->paintball_spawner.shooting_speed, 0, 100, " %.1f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Shoot spread", &fountain_spawner->paintball_spawner.shooting_spread, 0, 3, " %.2f", ImGuiSliderFlags_AlwaysClamp);
						unsigned int rps_min = 0, rps_max = 1000;
						ImGui::SliderScalar("Rounds per Sec", ImGuiDataType_U32, &fountain_spawner->paintball_spawner.rounds_per_second, &rps_min, &rps_max, " %d", ImGuiSliderFlags_AlwaysClamp);

						ImGui::Unindent();
						ImGui::PopID();
					}
				}
				ImGui::PopID();
				ImGui::Unindent();
			}

			// Light settings
			if (ImGui::CollapsingHeader("Lights"))
			{
				ImGui::Indent();
				ImGui::PushID(&point_lights);
				if (ImGui::CollapsingHeader("Pointlights"))
				{
					if (ImGui::BeginCombo("Selected light##selcombo", "0")) // The second parameter is the label previewed before opening the combo.
					{
						for (int n = 0; n < point_lights.size(); n++)
						{
							bool is_selected = (currentLight == point_lights[n]); // You can store your selection however you want, outside or inside your objects
							if (ImGui::Selectable(std::to_string(n).c_str(), is_selected))
								currentLight = point_lights[n];
						}
						ImGui::EndCombo();
					}
					if (ImGui::BeginCombo("Shadowmap resolution##rescombo", std::to_string(current_pl_res).c_str())) // The second parameter is the label previewed before opening the combo.
					{
						for (int n = 0; n < shadowmap_res.size(); n++)
						{
							bool is_selected = (current_pl_res == shadowmap_res[n]); // You can store your selection however you want, outside or inside your objects
							if (ImGui::Selectable(std::to_string(shadowmap_res[n]).c_str(), is_selected))
							{
								current_pl_res = shadowmap_res[n];
								for (auto& pl : point_lights)
								{
									pl->resize_shadowmap(current_pl_res);
								}
							}
						}
						ImGui::EndCombo();
					}
					for (int i = 0; i < point_lights.size(); i++)
					{
						ImGui::PushID(i);
						std::string light_label = "Point light n." + std::to_string(i);
						ImGui::Separator(); ImGui::Text(light_label.c_str());
						ImGui::SliderFloat3("Pos", glm::value_ptr(point_lights[i]->position), -20, 20, "%.2f", 1);
						ImGui::SliderFloat("Intensity", &point_lights[i]->intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::ColorEdit4("Color", glm::value_ptr(point_lights[i]->color));

						ImGui::SliderFloat("Att_const", &point_lights[i]->attenuation_constant, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Att_lin", &point_lights[i]->attenuation_linear, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Att_quad", &point_lights[i]->attenuation_quadratic, 0, 0.1f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

						ImGui::PopID();
					}
				}
				ImGui::PopID();

				ImGui::PushID(&dir_lights);
				if (ImGui::CollapsingHeader("Dir lights"))
				{
					if (ImGui::BeginCombo("Shadowmap resolution##rescombo", std::to_string(current_dl_res).c_str())) // The second parameter is the label previewed before opening the combo.
					{
						for (int n = 0; n < shadowmap_res.size(); n++)
						{
							bool is_selected = (current_dl_res == shadowmap_res[n]); // You can store your selection however you want, outside or inside your objects
							if (ImGui::Selectable(std::to_string(shadowmap_res[n]).c_str(), is_selected))
							{
								current_dl_res = shadowmap_res[n];
								for (auto& dl : dir_lights)
								{
									dl->resize_shadowmap(current_dl_res);
								}
							}
						}
						ImGui::EndCombo();
					}
					for (int i = 0; i < dir_lights.size(); i++)
					{
						ImGui::PushID(i);
						std::string light_label = "Directional light n." + std::to_string(i);
						ImGui::Separator(); ImGui::Text(light_label.c_str());
						ImGui::SliderFloat3("Dir", glm::value_ptr(dir_lights[i]->direction), -1, 1, "%.2f", 1);
						ImGui::SliderFloat("Intensity", &dir_lights[i]->intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::ColorEdit4("Color", glm::value_ptr(dir_lights[i]->color));

						ImGui::SliderFloat("Dist bias", &dir_lights[i]->shadowmap_settings.distance_bias, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Near ", &dir_lights[i]->shadowmap_settings.frustum_near, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Far ", &dir_lights[i]->shadowmap_settings.frustum_far, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::SliderFloat("Frust ", &dir_lights[i]->shadowmap_settings.frustum_size, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::PopID();
					}
				}
				ImGui::PopID();
				ImGui::Unindent();
			}

			// Other settings
			if (ImGui::CollapsingHeader("other coefficients and scales"))
			{
				ImGui::Text("Scene");
				ImGui::Checkbox("Use frustum culling", &main_scene.use_frustum_culling);

				ImGui::Separator(); ImGui::Text("Paintspace proj params");
				ImGui::SliderFloat("Near plane##paint", &PaintballComponent::paint_near_plane, 0, 2, " % .2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Far plane##paint", &PaintballComponent::paint_far_plane, 0, 100, " % .2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Distance bias##paint", &PaintballComponent::distance_bias, 0, 100, " % .2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Separator(); ImGui::Text("Floor");
				ImGui::SliderFloat("Repeat tex##floor", &floor_plane->material->uv_repeat, 0, 3000, " % .1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Parallax Height Scale##floor", &floor_plane->material->parallax_heightscale, 0, 0.5, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Paint normal bias##floor", &floor_plane->material->detail_normal_bias, 0, 0.5, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Paint threshold##floor", &floor_plane->material->detail_alpha_threshold, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Separator(); ImGui::Text("Cube");
				ImGui::SliderFloat("Repeat tex##cube", &cube->material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Parallax Height Scale##cube", &cube->material->parallax_heightscale, 0, 0.5, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Paint threshold##cube", &cube->material->detail_alpha_threshold, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Separator(); ImGui::Text("Front wall");
				ImGui::SliderFloat("Repeat tex##front", &wall_plane->material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Parallax Height Scale##front", &wall_plane->material->parallax_heightscale, 0, 0.5, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Paint threshold##front", &wall_plane->material->detail_alpha_threshold, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Separator(); ImGui::Text("Bunny");
				ImGui::SliderFloat("Repeat tex##bunny", &bunny->material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Parallax Height Scale##bunny", &bunny->material->parallax_heightscale, 0, 0.5, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Paint threshold##bunny", &bunny->material->detail_alpha_threshold, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			}
		}
		ImGui::End();

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion imgui_draw

#pragma region loop_cleanup
		// Swap buffers and cleanup
		main_scene.remove_marked();
		glfwSwapBuffers(glfw_window);
#pragma endregion loop_cleanup
	}

#pragma endregion rendering_loop

#pragma region post-loop_cleanup
	std::cout << " Avg.fps : " << (avg_fps) << " | Avg. frametime (ms) :" << avg_ms_per_frame;
	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
#pragma endregion post-loop_cleanup
	
	return 0;
}