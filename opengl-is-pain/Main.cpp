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
#include "utils/random.h"

#include "utils/scene/camera.h"
#include "utils/scene/entity.h"
#include "utils/scene/scene.h "
#include "utils/scene/light.h "

#include "utils/components/rigidbody_component.h"
#include "utils/components/paintable_component.h"
#include "utils/components/paintball_component.h"

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

// window
Window* wdw_ptr;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void setup_input_keys();

// input related parameters
float cursor_x, cursor_y;
bool firstMouse = true;
bool capture_mouse = true;

// Scene
Scene main_scene;
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
PhysicsEngine<Entity> physics_engine;
float maxSecPerFrame = 1.0f / 60.0f;
float capped_deltaTime;

// Paint
glm::vec4 paint_color{1.f, 1.f, 0.f, 1.f};
float paintball_size = 0.1f;
bool autofire = true;

// Random
utils::random::generator rng;

// Temporary 
GLint shadow_texture_unit = 5;
Entity* sphere_ptr;
Model* sphere_model_ptr;
Material* bullet_material_ptr;


// TODOs 

//////////////////////////////////////////

// HELPERS

// INPUT
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

// Creates and launches a paintball, if precise is true then there will be no spread among fired bullets 
std::function<void(bool)> los_paintball = [&](bool precise)
{
	Entity* bullet = main_scene.emplace_instanced_entity("bullets", "bullet", "This is a paintball", *sphere_model_ptr, *bullet_material_ptr);
	bullet->set_position(main_scene.current_camera->position() + main_scene.current_camera->forward() - glm::vec3{0, 0.5f, 0});
	bullet->set_size(glm::vec3(paintball_size));

	// setting up collision mask for paintball rigidbodies (to avoid colliding with themselves)
	CollisionFilter paintball_cf{ 1 << 7, ~(1 << 7) }; // this means "Paintball group is 1 << 7 (128), mask is everything but paintball group (inverse of the group bit)"

	bullet->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 1.0f, 1.0f, {ColliderShape::SPHERE, glm::vec3{1}} }, paintball_cf, true);
	bullet->emplace_component<PaintballComponent>(paint_color);

	bullet->init();

	float speed_modifier = 20.f;
	glm::vec3 spread_modifier {0};

	if (!precise)
	{
		float speed_bias = 2.0f;
		float spread_bias = 1.5f;
		float speed_spread = rng.get_float(-1, 1) * speed_bias; // get float between -bias and +bias
		speed_modifier = 20.f + speed_spread;
		spread_modifier = normalize(glm::vec3{rng.get_float(-1, 1), rng.get_float(-1, 1), rng.get_float(-1, 1)}) * spread_bias;
	}

	glm::vec3 shoot_dir = main_scene.current_camera->forward() * speed_modifier + spread_modifier;
	btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
	bullet->get_component<RigidBodyComponent>()->rigid_body->applyCentralImpulse(impulse);
};

void setup_input_keys()
{
	// Pressed input
	Input::instance().add_onPressed_callback(GLFW_KEY_W, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::FORWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_S, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::BACKWARD, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_A, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::LEFT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_D, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::RIGHT, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_Q, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::DOWN, deltaTime); });
	Input::instance().add_onPressed_callback(GLFW_KEY_E, [&]() { main_scene.current_camera->ProcessKeyboard(Camera::Directions::UP, deltaTime); });

	Input::instance().add_onPressed_callback(GLFW_KEY_LEFT , [&]() { currentLight->position.x -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_RIGHT, [&]() { currentLight->position.x += mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_UP   , [&]() { currentLight->position.z -= mov_light_speed * deltaTime; });
	Input::instance().add_onPressed_callback(GLFW_KEY_DOWN , [&]() { currentLight->position.z += mov_light_speed * deltaTime; });

	Input::instance().add_onPressed_callback(GLFW_MOUSE_BUTTON_RIGHT, [&]()
		{
			// This control is a workaround to avoid triggering both onRelease and onPressed callbacks at the same time
			if (!autofire) return;

			// Fire precisely since it's single fire
			los_paintball(!autofire);
		});

	// Toggled input
	Input::instance().add_onRelease_callback(GLFW_KEY_ESCAPE  , [&]() { wdw_ptr->close(); });
	Input::instance().add_onRelease_callback(GLFW_KEY_LEFT_ALT, [&]() { capture_mouse = !capture_mouse; });
	Input::instance().add_onRelease_callback(GLFW_KEY_SPACE   , [&]() { main_scene.current_camera->toggle_fly(); });
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
	Input::instance().add_onRelease_callback(GLFW_MOUSE_BUTTON_RIGHT, [&]()
		{
			// This control is a workaround to avoid triggering both onRelease and onPressed callbacks at the same time
			if (autofire) return;

			// Fire with spread since it's autofire
			los_paintball(!autofire);
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
		main_scene.current_camera->ProcessMouseMovement(x_offset, y_offset);
}

/////////////////// MAIN function ///////////////////////
int main()
{
	Window wdw
	{
		Window::window_create_info
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
	Window::window_size ws = wdw.get_size();
	float width = gsl::narrow<float>(ws.width), height = gsl::narrow<float>(ws.height);

	// Callbacks linking with glfw
	glfwSetKeyCallback(glfw_window, key_callback);
	glfwSetCursorPosCallback(glfw_window, mouse_pos_callback);
	glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);

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
	Camera main_camera = Camera{ glm::vec3{0,0,5} }, topdown_camera;
	glm::mat4 view = main_camera.viewMatrix();
	glm::mat4 projection = main_camera.projectionMatrix();

	// Scene setup
	main_scene.current_camera = &main_camera;

#pragma endregion shader_setup

#pragma region shader_setup
	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	Shader basic_shader      { "shaders/text/generic/basic.vert" ,"shaders/text/generic/basic.frag", 4, 3 };
	Shader basic_mvp_shader  { "shaders/text/generic/mvp.vert", "shaders/text/generic/basic.frag", 4, 3 };
	Shader basic_color_shader{ "shaders/text/generic/mvp.vert" , "shaders/text/generic/fullcolor.frag", 4, 3 };
	Shader debug_shader      { "shaders/text/generic/mvp.vert", "shaders/text/generic/fullcolor.frag", 4, 3 };
	Shader default_lit       { "shaders/text/default_lit.vert", "shaders/text/default_lit.frag", 4, 3, nullptr, utils_shaders };
	Shader textured_shader   { "shaders/text/generic/textured.vert" , "shaders/text/generic/textured.frag", 4, 3 };
	Shader tex_depth_shader  { "shaders/text/generic/textured.vert" , "shaders/text/generic/textured_depth.frag", 4, 3 };
	Shader shadowmap_shader  { "shaders/text/generic/shadow_map.vert" , "shaders/text/generic/shadow_map.frag", 4, 3 };
	Shader painter_shader    { "shaders/text/generic/texpainter.vert" , "shaders/text/generic/texpainter.frag", 4, 3 };

	std::vector <std::reference_wrapper<Shader>> lit_shaders, all_shaders;
	lit_shaders.push_back(default_lit);
	all_shaders.push_back(basic_mvp_shader); all_shaders.push_back(default_lit);

	// Lights setup 
	Light::ShadowMapSettings sm_settings;
	sm_settings.resolution = 1024;
	sm_settings.shader = &shadowmap_shader;

	PointLight pl1{ glm::vec3{-8.0f, 2.0f, 2.5f}, glm::vec4{1, 0, 1, 1}, 1 };
	PointLight pl2{ glm::vec3{-8.0f, 2.0f, 7.5f}, glm::vec4{0, 1, 1, 1}, 1 };
	DirectionalLight dl1{ glm::vec3{ -1, -1, -1 }, glm::vec4{1, 1, 1, 1}, 1, sm_settings};
	DirectionalLight dl2{ glm::vec3{ -1, -1, 0 }, glm::vec4{1, 1, 1, 1}, 1, sm_settings};
	DirectionalLight dl3{ glm::vec3{ 1, -1, 0 }, glm::vec4{1, 1, 1, 1}, 1, sm_settings};

	std::vector<PointLight*> point_lights;
	point_lights.push_back(&pl1); point_lights.push_back(&pl2);

	std::vector<DirectionalLight*> dir_lights;
	dir_lights.push_back(&dl1); dir_lights.push_back(&dl2);dir_lights.push_back(&dl3);

	currentLight = point_lights[0];

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
		lit_shader.setUint("nDirLights", gsl::narrow<unsigned int>(dir_lights.size()));
	}
#pragma endregion shader_setup

#pragma region materials_setup
	// Textures setup
	Texture greybricks_diffuse_tex{ "textures/brickwall.jpg" }, greybricks_normal_tex{ "textures/brickwall_normal.jpg" };
	Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" },
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex{ "textures/bricks2_disp.jpg" };
	Texture splat_tex{ "textures/uniform_splat_inv.png" }, splat_normal_tex{"textures/splat_normal.jpg"};

	// Shared materials
	Material redbricks_mat{ default_lit };
	redbricks_mat.diffuse_map = &redbricks_diffuse_tex; redbricks_mat.normal_map = &redbricks_normal_tex; redbricks_mat.displacement_map = &redbricks_depth_tex;
	redbricks_mat.parallax_heightscale = 0.05f;

	Material grey_bricks{ default_lit };
	grey_bricks.diffuse_map = &greybricks_diffuse_tex; grey_bricks.normal_map = &greybricks_normal_tex;

	Material sph_mat{ default_lit };

	Material basic_mat{ basic_mvp_shader };

	// Specific materials (ad hoc)
	Material wall_material { redbricks_mat };
	wall_material.uv_repeat = 20.f;

	Material left_room_lwall_material { wall_material };
	Material left_room_rwall_material { wall_material };
	Material left_room_bwall_material { wall_material };

	Material floor_material{ grey_bricks }; //TODO LA UV REPEAT E I SUOI ESTREMI DEVONO DIPENDERE DALLA DIMENSIONE DELLA TEXTURE E/O DALLA SIZE DELLA TRANSFORM?
	floor_material.uv_repeat = 0.75f * std::max(floor_material.diffuse_map->width(), floor_material.diffuse_map->height());

	Material cube_material { redbricks_mat };
	cube_material.uv_repeat = 3.f;

	Material test_cube_material{ default_lit };

	Material bullet_material{ default_lit };

#pragma endregion materials_setup

	// Entities setup
	Model plane_model{ "models/quad.obj" }, cube_model{ "models/cube.obj" }, sphere_model{ "models/sphere.obj" }, bunny_model{ "models/bunny.obj" };

	Entity* cube        = main_scene.emplace_entity("cube", "brick_cube", cube_model, cube_material);
	Entity* test_cube   = main_scene.emplace_entity("test_cube", "test_cube", cube_model, test_cube_material);
	Entity* floor_plane = main_scene.emplace_entity("floor", "floorplane", plane_model, floor_material);
	Entity* wall_plane  = main_scene.emplace_entity("wall", "wallplane", plane_model, wall_material);
	Entity* sphere      = main_scene.emplace_entity("sphere", "sphere", sphere_model, sph_mat);

	Entity* left_room_lwall = main_scene.emplace_entity("wall", "left_room_leftwall", plane_model, left_room_lwall_material);
	Entity* left_room_rwall = main_scene.emplace_entity("wall", "left_room_rightwall", plane_model, left_room_rwall_material);
	Entity* left_room_bwall = main_scene.emplace_entity("wall", "left_room_backwall", plane_model, left_room_bwall_material);
	//Entity* bunny       = main_scene.emplace_entity("bunny", "buny", bunny_model, sph_mat);

	sphere_ptr = sphere;

	Model triangle_mesh{ Mesh::simple_triangle_mesh() };
	Model quad_mesh{ Mesh::simple_quad_mesh() };
	Entity cursor{ "cursor", triangle_mesh, basic_mat };

	// Lightcubes
	std::vector<Material> lightcube_materials; lightcube_materials.resize(point_lights.size());
	std::vector<Entity*> lightcube_entites; lightcube_entites.resize(point_lights.size());
	for (size_t i = 0; i < point_lights.size(); i++)
	{
		Material lightcolor { default_lit };
		lightcolor.ambient_color = point_lights[i]->color;
		lightcolor.kA = 1; lightcolor.kD = 0; lightcolor.kS = 0;
		lightcolor.receive_shadows = false;
		lightcube_materials[i] = lightcolor;

		lightcube_entites[i] = main_scene.emplace_entity("light_cube", "light_cube", cube_model, lightcube_materials[i]);
	}

	// Entities setup in scene
	scene_setup = [&]()
	{
		floor_plane->set_position(glm::vec3(0.0f, -1.0f, 0.0f));
		floor_plane->set_size(glm::vec3(100.0f, 0.1f, 100.0f));

		wall_plane->set_position(glm::vec3(0.0f, 4.0f, -10.0f));
		wall_plane->set_rotation(glm::vec3(90.0f, 0.0f, 0.f));
		wall_plane->set_size(glm::vec3(10.0f, 0.1f, 5.0f));

		left_room_lwall->set_position(glm::vec3(-10.0f, 4.0f, 0.0f));
		left_room_lwall->set_rotation(glm::vec3(90.0f, 0.0f, 0.f));
		left_room_lwall->set_size(glm::vec3(5.0f, 0.1f, 5.0f));

		left_room_rwall->set_position(glm::vec3(-10.0f, 4.0f, 10.0f));
		left_room_rwall->set_rotation(glm::vec3(90.0f, 0.0f, 0.f));
		left_room_rwall->set_size(glm::vec3(5.0f, 0.1f, 5.0f));

		left_room_bwall->set_position(glm::vec3(-15.0f, 4.0f, 5.f));
		left_room_bwall->set_rotation(glm::vec3(90.0f, 90.0f, 0.f));
		left_room_bwall->set_size(glm::vec3(5.0f, 0.1f, 5.0f));

		cube->set_position(glm::vec3(0.0f, 3.0f, 0.0f));
		cube->set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));

		test_cube->set_position(glm::vec3(-12.0f, 0.0f, 5.f));

		sphere->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
		sphere->set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
		sphere->set_size(glm::vec3(0.25f));

		//bunny->set_position(glm::vec3(-5.0f, 3.0f, 0.0f));
		//bunny->set_size(glm::vec3(1.f));

		for (auto& lightcube_entity : lightcube_entites)
		{
			lightcube_entity->set_size(glm::vec3(0.25f));
		}

		cursor.set_size(glm::vec3(3.0f));
	};
	scene_setup();

	

	// Physics setup
	GLDebugDrawer phy_debug_drawer{ main_camera, debug_shader };
	physics_engine.addDebugDrawer(&phy_debug_drawer);
	physics_engine.set_debug_mode(0);

	floor_plane->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{100.0f, 0.01f, 100.0f}}}, false );
	wall_plane ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	test_cube  ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 0.1f, 0.1f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	cube       ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 0.1f, 0.1f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	sphere     ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 1.0f, 1.0f, {ColliderShape::SPHERE, glm::vec3{1}} }, true);

	left_room_lwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	left_room_rwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);
	left_room_bwall->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 0.0f, 3.0f, 0.5f, {ColliderShape::BOX,    glm::vec3{1}} }, true);

	std::vector<glm::vec3> bunny_mesh_vertices = bunny_model.get_vertices_positions();
	//bunny      ->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 10.0f, 1.0f, 1.0f,
	//	ColliderShapeCreateInfo{ ColliderShape::HULL, glm::vec3{1}, &bunny_mesh_vertices } }, false);

	test_cube      ->emplace_component<PaintableComponent>(painter_shader, 128, 128, &splat_tex, &splat_normal_tex);
	wall_plane     ->emplace_component<PaintableComponent>(painter_shader, 1024, 1024, &splat_tex, &splat_normal_tex);
	cube           ->emplace_component<PaintableComponent>(painter_shader, 128, 128, &splat_tex, &splat_normal_tex);
	floor_plane    ->emplace_component<PaintableComponent>(painter_shader, 4096, 4096, &splat_tex, &splat_normal_tex);

	left_room_lwall->emplace_component<PaintableComponent>(painter_shader, 1024, 1024, &splat_tex, &splat_normal_tex);
	left_room_rwall->emplace_component<PaintableComponent>(painter_shader, 1024, 1024, &splat_tex, &splat_normal_tex);
	left_room_bwall->emplace_component<PaintableComponent>(painter_shader, 1024, 1024, &splat_tex, &splat_normal_tex);

	// Framebuffers
	Framebuffer map_framebuffer{ ws.width, ws.height, Texture::FormatInfo{GL_RGB, GL_RGB, GL_UNSIGNED_BYTE}, Texture::FormatInfo{GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} };

	// Bullet stuff
	sphere_model_ptr = &sphere_model;
	bullet_material_ptr = &bullet_material;

	main_scene.init();

	while (wdw.is_open())
	{
#pragma region setup_loop
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		float currentFrame = gsl::narrow_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		capped_deltaTime = deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame;
		lastFrame = currentFrame;

		main_scene.current_camera = &main_camera;

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
#pragma endregion update_world

#pragma region shadow_pass
		
		for (auto& light : dir_lights)
		{
			light->compute_shadowmap(main_scene); // TODO each light has its own lightspacematrix, thus should go in the array of light attributes in the shader
		}
		
		//debug draw a depth texture
		const Texture& computed_shadowmap = dir_lights[dir_lights.size()-1]->get_shadowmap();
		glViewport(0, 0, 256, 256);
		tex_depth_shader.bind();
		glActiveTexture(GL_TEXTURE0);
		computed_shadowmap.bind();
		quad_mesh.draw();
		textured_shader.unbind();
		glViewport(0, 0, ws.width, ws.height);

		// Update lit shaders to add the computed shadowmap(s)
		std::vector<GLint> locs;
		for (Shader& lit_shader : lit_shaders)
		{
			lit_shader.bind();
			for (int i = 0; i < dir_lights.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0+shadow_texture_unit+i);
				dir_lights[i]->get_shadowmap().bind();
				locs.push_back(shadow_texture_unit + i);
			}
			lit_shader.setIntV("directional_shadow_maps", gsl::narrow<int>(locs.size()), locs.data());
			lit_shader.unbind();
		}

#pragma endregion shadow_pass

#pragma region draw_world
		// Render scene
		main_scene.draw();
		//main_scene.instanced_draw(bullet_material);

#pragma endregion draw_world

#pragma region map_draw
		// MAP
		map_framebuffer.bind();
		glClearColor(0.26f, 0.98f, 0.26f, 1.0f); //the "clear" color for the default frame buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float topdown_height = 20;
		topdown_camera.set_position(main_camera.position() + glm::vec3{ 0, topdown_height, 0 });
		topdown_camera.lookAt(main_camera.position());
		main_scene.current_camera = &topdown_camera;

		// Update camera info since we swapped to topdown
		for (Shader& shader : all_shaders)
		{
			shader.bind();
			shader.setVec3("wCameraPos", main_scene.current_camera->position());
			shader.setMat4("viewMatrix", main_scene.current_camera->viewMatrix());
		}

		// Redraw all scene objects from map pov
		main_scene.draw();

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
		std::string fps_counter = "Approximate FPS: " + std::to_string(ImGui::GetIO().Framerate);
		ImGui::Text(fps_counter.c_str());
		if (ImGui::CollapsingHeader("Coefficients and scales"))
		{
			ImGui::ColorEdit4 ("Paint Color",    glm::value_ptr(paint_color));
			ImGui::SliderFloat("Paintball size", &paintball_size, 0, 1, " %.1f", ImGuiSliderFlags_AlwaysClamp);
			
			ImGui::Checkbox   ("Automatic firing", &autofire);
			ImGui::Separator(); ImGui::Text("Normal");
			ImGui::SliderFloat("Repeat tex##norm", &floor_plane->material->uv_repeat, 0, 3000, " % .1f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Separator(); ImGui::Text("Parallax");
			ImGui::SliderFloat("Height Scale", &cube->material->parallax_heightscale, 0, 0.5, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SliderFloat("Repeat tex##prlx", &cube->material->uv_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		}
		if (ImGui::CollapsingHeader("Lights"))
		{
			for (size_t i = 0; i < point_lights.size(); i++)
			{
				ImGui::PushID(&point_lights + i);
				std::string light_label = "Point light n." + std::to_string(i);
				ImGui::Separator(); ImGui::Text(light_label.c_str());

				ImGui::SliderFloat("Intensity", &point_lights[i]->intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::ColorEdit4 ("Color", glm::value_ptr(point_lights[i]->color));
				ImGui::SliderFloat3("Pos" , glm::value_ptr(point_lights[i]->position), -20, 20, "%.2f", 1);
				ImGui::SliderFloat("Att_const", &point_lights[i]->attenuation_constant, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Att_lin"  , &point_lights[i]->attenuation_linear, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Att_quad" , &point_lights[i]->attenuation_quadratic, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);

				ImGui::PopID();
			}
			for (size_t i = 0; i < dir_lights.size(); i++)
			{
				ImGui::PushID(&dir_lights + i);
				std::string light_label = "Directional light n." + std::to_string(i);
				ImGui::Separator(); ImGui::Text(light_label.c_str());
				ImGui::SliderFloat3("Dir",   glm::value_ptr(dir_lights[i]->direction), -1, 1, "%.2f", 1);
				ImGui::SliderFloat ("Intensity",           &dir_lights[i]->intensity, 0, 1, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::ColorEdit4  ("Color", glm::value_ptr(dir_lights[i]->color));
				
				ImGui::SliderFloat("Dist bias", &dir_lights[i]->shadowmap_settings.distance_bias, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Near ", &dir_lights[i]->shadowmap_settings.frustum_near, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Far ", &dir_lights[i]->shadowmap_settings.frustum_far, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Frust ", &dir_lights[i]->shadowmap_settings.frustum_size, 0, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::PopID();
			}
		}

		ImGui::End();

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion imgui_draw

		// Swap buffers and cleanup
		main_scene.remove_marked();
		glfwSwapBuffers(glfw_window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return 0;
}