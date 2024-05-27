#include <glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils/window.h"
#include "utils/shader.h"
#include "utils/scene/camera.h"
#include "utils/model.h "
#include "utils/scene/entity.h"
#include "utils/input.h"

#include <iostream>

namespace
{
	using namespace engine::input;
    using namespace engine::scene;
    using namespace engine::resources;
    using namespace utils::graphics::opengl;


	void k_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
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

	void m_callback(GLFWwindow* window, double x_pos, double y_pos)
	{
	}

	void setup_keys(Window& wdw)
	{
		Input::instance().add_onRelease_callback(GLFW_KEY_ESCAPE, [&]() { wdw.close(); });
	}
}

int mainz()
{
	Window wdw
	{
		Window::window_create_info
		{
			{ "Quick test" }, //.title
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

	GLFWwindow* glfw_window = wdw.get();
	Window::window_size ws = wdw.get_size();
	float width = gsl::narrow<float>(ws.width), height = gsl::narrow<float>(ws.height);

	// Callbacks linking with glfw
	glfwSetKeyCallback(glfw_window, k_callback);
	glfwSetCursorPosCallback(glfw_window, m_callback);

	// Setup keys
	setup_keys(wdw);

	Shader painter_shader{ "jj", "shaders/text/generic/texpainter.vert" ,"shaders/text/generic/texpainter.frag", 4, 3 };
	Shader mvp{ "mvp", "shaders/text/generic/mvp.vert", "shaders/text/generic/fullcolor.frag", 4 , 3 };
	Model cube_model{ "models/cube.obj" }, sphere_model{ "models/sphere.obj" }, bunny_model{ "models/bunny.obj" };
	
	Camera cam{ glm::vec3{0, 0, 5.f} };

	float avg_fps = 1.f, alpha = 0.9f;
	float deltaTime = 0.f, lastFrameTime = 0.f;
	while (wdw.is_open())
	{
		float currentFrameTime = gsl::narrow_cast<float>(glfwGetTime());
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		avg_fps  = alpha * avg_fps + (1.0f - alpha) * (1.f / deltaTime);
		std::cout << "Avg.fps :" << (avg_fps);
		std::cout << "\r" << std::flush;

		// Check is an I/O event is happening
		glfwPollEvents();
		Input::instance().process_pressed_keys();

		// Clear the frame and z buffer
		glClearColor(0.26f, 0.46f, 0.98f, 1.0f); //the "clear" color for the default frame buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		mvp.bind();
		{
			mvp.setMat4("viewMatrix", cam.viewMatrix());
			mvp.setMat4("projectionMatrix", cam.projectionMatrix());
			mvp.setVec3("colorIn", glm::vec3{ 1.f, 1.f, 0 });
			cube_model.draw();
		}
		mvp.unbind();

		glfwSwapBuffers(glfw_window);
	}

	return 0;
}