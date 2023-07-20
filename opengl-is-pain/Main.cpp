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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/camera.h"
#include "utils/object.h"
#include "utils/light.h "
#include "utils/window.h"
#include "utils/texture.h"

namespace ugl = utils::graphics::opengl;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void process_input();

GLfloat lastX, lastY;
bool firstMouse = true;
bool keys[1024];
bool capture_mouse = false;

ugl::Camera camera(glm::vec3(0, 0, 7), GL_TRUE);

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
	
	// Shader setup
	std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	ugl::Shader light_shader{ "shaders/BP_normal_mapping.vert", "shaders/BP_normal_mapping.frag", 4, 3, nullptr, utils_shaders};
	ugl::Shader basic_shader{ "shaders/mvp.vert", "shaders/basic.frag", 4, 3 };

	// Camera setup
	glm::mat4 projection = glm::perspective(45.0f, width / height, 0.1f, 10000.0f);
	glm::mat4 view = glm::mat4(1);

	// Objects setup
	ugl::Model plane_model{ "models/plane.obj" }, bunny_model{ "models/cube.obj" };
	ugl::Object<ugl::Model> plane{ plane_model }, cube{ bunny_model };
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
	ugl::Object<ugl::Mesh> cursor{ triangle_mesh };

	std::vector<ugl::Object<ugl::Model>*> scene_objects;
	scene_objects.push_back(&plane); scene_objects.push_back(&cube);

	// Scene material/lighting setup 
	glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 1.0f, 1.0f }, specular{ 1.0f, 1.0f, 1.0f };
	GLfloat kD = 0.5f, kS = 0.4f, kA = 0.1f; // Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
	GLfloat shininess = 25.f;
	GLfloat alpha = 0.2f;
	GLfloat F0 = 0.9f;

	std::vector<glm::vec3> shading_attrs_colors{ ambient, diffuse, specular };
	std::vector<float> shading_attrs_coeff{ kD, kS, kA, shininess, alpha, F0 };
	
	// Lights setup 
	ugl::PointLight pl1{ glm::vec3{-20.f, 10.f, 10.f}, glm::vec4{1,0,1,1},1 };
	ugl::PointLight pl2{ glm::vec3{ 20.f, 10.f, 10.f}, glm::vec4{1,1,1,1},1 };

	std::vector<ugl::PointLight> point_lights;
	point_lights.push_back(pl1); point_lights.push_back(pl2);
	currentLight = &point_lights[0];

	// Textures setup
	Texture uv_tex{ "textures/UV_Grid_Sm.png" }, soil_tex { "textures/SoilCracked.png" };
	Texture wall_diffuse_tex{ "textures/brickwall.jpg" }, wall_normal_tex{ "textures/brickwall_normal.jpg" };

	//GLuint currentSubroutineIndex;
	//currentSubroutineIndex = light_shader.getSubroutineIndex(GL_FRAGMENT_SHADER, "BlinnPhong");
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &currentSubroutineIndex);
	
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
		view = camera.GetViewMatrix();

		// we "clear" the frame and z buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
		if (spinning)
			orientationY += (deltaTime * spin_speed);

		light_shader.use();

		light_shader.setMat4("projectionMatrix", projection);
		light_shader.setMat4("viewMatrix", view);
		light_shader.setVec3("cameraPos", camera.position());

		// LIGHTING 
		light_shader.setUint("nPointLights", point_lights.size());
		for (size_t i = 0; i < point_lights.size(); i++)
		{
			point_lights[i].setup(light_shader, i);
		}
		light_shader.setVec3("ambient", ambient);
		light_shader.setVec3("diffuse", diffuse);
		light_shader.setVec3("specular", specular);

		light_shader.setFloat("kA", kA);
		light_shader.setFloat("kD", kD);
		light_shader.setFloat("kS", kS);

		light_shader.setFloat("shininess", shininess);
		light_shader.setFloat("alpha", alpha);

		// OBJECTS
		ws = wdw.get_size();
		glViewport(0, 0, ws.width, ws.height); // we render objects in full screen

		// Plane
		plane.translate(glm::vec3(0.0f, -1.0f, 0.0f));
		plane.scale(glm::vec3(10.0f, 1.0f, 10.0f));

		wall_diffuse_tex.activate();
		wall_normal_tex.activate();
		light_shader.setInt("diffuse_tex", wall_diffuse_tex.id);
		light_shader.setInt("normal_tex", wall_normal_tex.id);
		light_shader.setFloat("repeat", 90.f);

		plane.draw(light_shader, view);

		// cube
		cube.translate(glm::vec3(0.0f, 0.0f, 0.0f));
		cube.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
		cube.scale(glm::vec3(0.2f));	// It's a bit too big for our scene, so scale it down

		wall_diffuse_tex.activate();
		wall_normal_tex.activate();
		light_shader.setInt("diffuse_tex", wall_diffuse_tex.id);
		light_shader.setInt("normal_tex", wall_normal_tex.id);
		light_shader.setFloat("repeat", 2.f);

		cube.draw(light_shader, view);

		// MAP
		glClear(GL_DEPTH_BUFFER_BIT); // clears depth information, thus everything rendered from now on will be on top https://stackoverflow.com/questions/5526704/how-do-i-keep-an-object-always-in-front-of-everything-else-in-opengl
		glViewport(ws.width - 200, ws.height - 150, 200, 150); // we render now into the smaller map viewport
		
		float topdown_height = 20;
		glm::mat4 topdown_view = glm::lookAt(camera.position() + glm::vec3{0, topdown_height, 0}, camera.position(), camera.forward());
		
		// Redraw all scene objects from map pov
		light_shader.setMat4("viewMatrix", topdown_view);

		uv_tex.activate();
		light_shader.setInt("diffuse_tex", uv_tex.id);
		light_shader.setFloat("repeat", 1.f);

		for (ugl::Object<ugl::Model>* o : scene_objects)
		{
			o->draw(light_shader, topdown_view);
		}

		// Prepare cursor shader
		glClear(GL_DEPTH_BUFFER_BIT);
		basic_shader.use();
		basic_shader.setMat4("projectionMatrix", projection);
		basic_shader.setMat4("viewMatrix", topdown_view);

		cursor.translate(camera.position());
		cursor.rotate_deg(90.f, { -1.0f, 0.0f, 0.0f });
		cursor.rotate_deg(camera.rotation().y + 90.f, { 0.0f, 0.0f, -1.0f });
		cursor.scale(glm::vec3(3.0f)); 

		cursor.draw(basic_shader, topdown_view);
		cursor.reset_transform();
		
		// Reset all objects transforms
		for (ugl::Object<ugl::Model>* o : scene_objects)
		{
			o->reset_transform();
		}

		// Swap buffers
		glfwSwapBuffers(glfw_window);
	}

	// cleanup
	basic_shader.dispose();
	light_shader.dispose();

	return 0;
}

//////////////////////////////////////////

// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// if ESC is pressed, we close the application
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// if C is pressed, capture the mouse cursor
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
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

	camera.ProcessMouseMovement(x_offset, y_offset);
}