// std libraries
#include <string>

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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/texture.h"

#include "utils/scene/camera.h"
#include "utils/scene/entity.h"
#include "utils/scene/light.h "

#include "utils/window.h"

#include "utils/components/texture_component.h"

namespace ugl = utils::graphics::opengl;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void process_input();

GLfloat lastX, lastY;
bool firstMouse = true;
bool keys[1024];
bool capture_mouse = true;

ugl::Camera camera;

// parameters for time computation
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
GLboolean spinning = GL_TRUE;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

ugl::PointLight* currentLight;
GLfloat mov_light_speed = 30.f;

#define MAX_POINT_LIGHTS 3
#define MAX_SPOT_LIGHTS  3
#define MAX_DIR_LIGHTS   3
#define MAX_LIGHTS MAX_POINT_LIGHTS+MAX_SPOT_LIGHTS+MAX_DIR_LIGHTS

void set_matrices(ugl::Shader& shader);
void set_light_attributes(ugl::Shader& shader);

// Scene material/lighting setup 
glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
GLfloat kD = 0.5f, kS = 0.4f, kA = 0.1f; // Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
GLfloat shininess = 25.f;
GLfloat alpha = 0.2f;
GLfloat F0 = 0.9f;

/////////////////// MAIN function ///////////////////////
int main()
{
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
			{ false   }, //.resizable
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
	
	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	ugl::Shader norm_map_shader{ "shaders/BP_normal_mapping.vert", "shaders/BP_normal_mapping.frag", 4, 3, nullptr, utils_shaders};
	//ugl::Shader light_shader{ "shaders/normal_mapping.vert", "shaders/normal_mapping.frag", 4, 3, nullptr, utils_shaders};
	ugl::Shader basic_shader{ "shaders/mvp.vert", "shaders/basic.frag", 4, 3 };
	ugl::Shader parallax_map_shader{ "shaders/BP_parallax_mapping.vert", "shaders/BP_parallax_mapping.frag", 4, 3, nullptr, utils_shaders };

	// Camera setup
	camera = ugl::Camera{ glm::vec3{0,0,5} };
	glm::mat4 view = camera.view_matrix();
	glm::mat4 projection = camera.projection_matrix();

	// Objects setup
	ugl::Model plane_model{ "models/plane.obj" }, bunny_model{ "models/cube.obj" };
	ugl::Entity<ugl::Model> plane{ plane_model }, cube{ bunny_model };
	ugl::Mesh triangle_mesh
	{
		std::vector<ugl::Vertex>
		{
			ugl::Vertex{ glm::vec3{-0.5f, -0.5f, 0.0f} /* position */ },
			ugl::Vertex{ glm::vec3{ 0.0f,  0.5f, 0.0f} /* position */ },
			ugl::Vertex{ glm::vec3{ 0.5f, -0.5f, 0.0f} /* position */ }
		},
		std::vector<GLuint>{0, 2, 1}
	};
	ugl::Entity<ugl::Mesh> cursor{ triangle_mesh };
	cursor.add_component<ugl::components::TextureComponent>("popo");

	std::vector<ugl::Entity<ugl::Model>*> scene_objects;
	scene_objects.push_back(&plane); scene_objects.push_back(&cube);

	//std::vector<glm::vec3> shading_attrs_colors{ ambient, diffuse, specular };
	//std::vector<float> shading_attrs_coeff{ kD, kS, kA, shininess, alpha, F0 };
	
	// Lights setup 
	ugl::PointLight pl1{ glm::vec3{-20.f, 10.f, 10.f}, glm::vec4{1,0,1,1},1 };
	ugl::PointLight pl2{ glm::vec3{ 20.f, 10.f, 10.f}, glm::vec4{1,1,1,1},1 };

	std::vector<ugl::PointLight> point_lights;
	point_lights.push_back(pl1); 
	point_lights.push_back(pl2);
	currentLight = &point_lights[0];

	// Textures setup
	Texture uv_tex{ "textures/UV_Grid_Sm.png" }, soil_tex { "textures/SoilCracked.png" };
	Texture wall_diffuse_tex{ "textures/brickwall.jpg" }, wall_normal_tex{ "textures/brickwall_normal.jpg" };
	Texture redbricks_diffuse_tex{ "textures/bricks2.jpg" }, 
		redbricks_normal_tex{ "textures/bricks2_normal.jpg" },
		redbricks_depth_tex{ "textures/bricks2_disp.jpg" };
	float parallax_heightscale = .1f;

	//GLuint currentSubroutineIndex;
	//currentSubroutineIndex = light_shader.getSubroutineIndex(GL_FRAGMENT_SHADER, "BlinnPhong");
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &currentSubroutineIndex);

	// Shader "constants" setup
	set_light_attributes(norm_map_shader);
	set_light_attributes(parallax_map_shader);
	float norm_map_repeat = 90.f, parallax_map_repeat = 3.f;

	// Objects setup
	plane.transform.set_position(glm::vec3(0.0f, -1.0f, 0.0f));
	plane.transform.set_size(glm::vec3(10.0f, 1.0f, 10.0f));

	cube.transform.set_position(glm::vec3(0.0f, 0.5f, 0.0f));
	cube.transform.set_size(glm::vec3(1.f));	// It's a bit too big for our scene, so scale it down

	cursor.transform.set_size(glm::vec3(3.0f));

	// Rendering loop: this code is executed at each frame
	while (wdw.is_open())
	{
		// we determine the time passed from the beginning
		// and we calculate the time difference between current frame rendering and the previous one
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (capture_mouse)
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(wdw.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		// Check is an I/O event is happening
		glfwPollEvents();
		process_input();

		// we "clear" the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
		if (spinning)
			orientationY += (deltaTime * spin_speed);

		// get matrices from camera
		view = camera.view_matrix();

		// OBJECTS
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height); // we render objects in full screen

		// Plane
		norm_map_shader.use();
		wall_diffuse_tex.activate();
		wall_normal_tex.activate();
		
		norm_map_shader.setMat4("viewMatrix", view);
		norm_map_shader.setMat4("projectionMatrix", projection);
		norm_map_shader.setVec3("wCameraPos", camera.position());

		// LIGHTING 
		norm_map_shader.setUint("nPointLights", point_lights.size());
		for (size_t i = 0; i < point_lights.size(); i++)
		{
			point_lights[i].setup(norm_map_shader, i);
		}

		norm_map_shader.setInt("diffuse_tex", wall_diffuse_tex.id);
		norm_map_shader.setInt("normal_tex", wall_normal_tex.id);
		norm_map_shader.setFloat("repeat", norm_map_repeat);
		norm_map_shader.setBool("use_normalmap", true);

		plane.draw(norm_map_shader, view);

		// cube
		cube.transform.rotate(glm::vec3(0.0f, 30.0f * deltaTime, 0.0f));
		
		parallax_map_shader.use();
		redbricks_diffuse_tex.activate();
		redbricks_normal_tex.activate();
		redbricks_depth_tex.activate();

		parallax_map_shader.setMat4("viewMatrix", view);
		parallax_map_shader.setMat4("projectionMatrix", projection);
		parallax_map_shader.setVec3("wCameraPos", camera.position());

		parallax_map_shader.setUint("nPointLights", point_lights.size());
		for (size_t i = 0; i < point_lights.size(); i++)
		{
			point_lights[i].setup(parallax_map_shader, i);
		}

		parallax_map_shader.setFloat("heightScale", parallax_heightscale);
		parallax_map_shader.setInt("diffuse_tex", redbricks_diffuse_tex.id);
		parallax_map_shader.setInt("normal_tex", redbricks_normal_tex.id);
		parallax_map_shader.setInt("depth_tex", redbricks_depth_tex.id);
		parallax_map_shader.setFloat("repeat", parallax_map_repeat);

		cube.draw(parallax_map_shader, view);

		// MAP
		glClear(GL_DEPTH_BUFFER_BIT); // clears depth information, thus everything rendered from now on will be on top https://stackoverflow.com/questions/5526704/how-do-i-keep-an-object-always-in-front-of-everything-else-in-opengl
		glViewport(ws.width - 200, ws.height - 150, 200, 150); // we render now into the smaller map viewport
		
		float topdown_height = 20;
		glm::vec3 new_cam_pos = camera.position() + glm::vec3{ 0, topdown_height, 0 };
		glm::mat4 topdown_view = glm::lookAt(new_cam_pos, camera.position(), camera.forward());
		
		// Redraw all scene objects from map pov
		norm_map_shader.use();
		norm_map_shader.setMat4("viewMatrix", topdown_view);
		norm_map_shader.setVec3("wCameraPos", new_cam_pos);

		uv_tex.activate();
		norm_map_shader.setInt("diffuse_tex", uv_tex.id);
		norm_map_shader.setFloat("repeat", 2.f);
		norm_map_shader.setBool("use_normalmap", false);

		for (ugl::Entity<ugl::Model>* o : scene_objects)
		{
			o->draw(norm_map_shader, topdown_view);
		}
		
		// Prepare cursor shader
		glClear(GL_DEPTH_BUFFER_BIT);
		basic_shader.use();
		basic_shader.setMat4("viewMatrix", topdown_view);
		basic_shader.setMat4("projectionMatrix", projection);

		cursor.transform.set_position(camera.position());
		//cursor.transform.rotate(camera.rotation().y + 90.f, { 0.0f, 0.0f, -1.0f });
		cursor.transform.set_rotation({ -90.0f, 0.0f, -camera.rotation().y - 90.f });

		cursor.draw(basic_shader, topdown_view);
		
		// Reset all objects transforms
		for (ugl::Entity<ugl::Model>* o : scene_objects)
		{
			//o->reset_transform();
		}

		// ImGUI window creation
		ImGui_ImplOpenGL3_NewFrame();// Tell OpenGL a new Imgui frame is about to begin
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Coefficients and scales");
		ImGui::Separator(); ImGui::Text("Normal");
		ImGui::SliderFloat("Repeat tex##norm", &norm_map_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Separator(); ImGui::Text("Parallax");
		ImGui::SliderFloat("Height Scale", &parallax_heightscale, 0, 2, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat("Repeat tex##prlx", &parallax_map_repeat, 0, 100, " % .1f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::End();

		// Renders the ImGUI elements
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		view = camera.view_matrix();

		// Swap buffers
		glfwSwapBuffers(glfw_window);
	}

	// Cleanup
	basic_shader.dispose();
	norm_map_shader.dispose();
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
	shader.use();
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

	// if LAlt is pressed, capture the mouse cursor
	if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
		capture_mouse = !capture_mouse;

	// if P is pressed, we start/stop the animated rotation of models
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		spinning = ~spinning;

	// if L is pressed, we activate/deactivate wireframe rendering of models
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		wireframe = ~wireframe;
		if (wireframe)
			// Draw in wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

void process_input()
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
	if (firstMouse)
	{
		lastX = x_pos;
		lastY = y_pos;
		firstMouse = false;
	}

	GLfloat x_offset = x_pos - lastX;
	GLfloat y_offset = lastY - y_pos;

	lastX = x_pos;
	lastY = y_pos;

	if(capture_mouse)
		camera.ProcessMouseMovement(x_offset, y_offset);
}