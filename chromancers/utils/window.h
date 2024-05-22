#pragma once

#include <iostream>

#include <glad.h> // OpenGL loader
#include <glfw/glfw3.h> // GLFW library to create window and to manage I/O

#include <gsl/gsl>

// Debug callbacks in a debug context are available from OpenGL 4.3+
inline void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

namespace utils::graphics::opengl
{
	// Class wrapping a GLFW window and setupping a openGL context
	class Window
	{
	public:
		struct window_create_info
		{
			std::string title{ "Window" };
			GLuint      gl_version_major{ 4 };
			GLuint      gl_version_minor{ 3 };
			GLuint      window_width   { 1200 };
			GLuint      window_height  { 900 };
			GLuint      viewport_width { window_width };
			GLuint      viewport_height{ window_height };
			GLboolean   resizable{ GLFW_FALSE };
			GLboolean   vsync    { GLFW_FALSE };
			GLboolean   gl_debug { GLFW_FALSE };
		};

		struct window_size
		{
			unsigned int width;
			unsigned int height;
		};


		Window(window_create_info create_info) : glfw_window{ init(create_info) } {}

		Window(GLFWwindow* window) : glfw_window{ window } {}

		           Window(const Window&  copy) = delete;
		Window& operator=(const Window&  copy) = delete;

		           Window(      Window&& move) noexcept = delete;
		Window& operator=(      Window&& move) noexcept = delete;

		~Window() { glfwTerminate(); }

		GLFWwindow* get()
		{
			return glfw_window;
		}

		window_size get_size()
		{
			int width, height;
			glfwGetWindowSize(glfw_window, &width, &height);
			return { gsl::narrow_cast<unsigned int>(width), gsl::narrow_cast<unsigned int>(height) };
		}

		bool is_open()
		{
			return !glfwWindowShouldClose(glfw_window);
		}

		void close()
		{
			glfwSetWindowShouldClose(glfw_window, GL_TRUE);
		}

	private:
		GLFWwindow* glfw_window;

		GLFWwindow* init(window_create_info create_info)
		{
			// Initialization of OpenGL context using GLFW
			glfwInit();
			// We set OpenGL specifications required for this application
			// In this case: 4.1 Core
			// If not supported by your graphics HW, the context will not be created and the application will close
			// N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
			// in relation also to the values indicated in these GLFW commands
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, create_info.gl_version_major);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, create_info.gl_version_minor);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, create_info.gl_debug);
			// we set if the window is resizable
			glfwWindowHint(GLFW_RESIZABLE, create_info.resizable);

			// we create the application's window
			GLFWwindow* window = glfwCreateWindow(create_info.window_width, create_info.window_height, create_info.title.c_str(), nullptr, nullptr);
			if (!window)
			{
				glfwTerminate();
				throw std::runtime_error{ "Failed to create GLFW window" };
			}

			glfwMakeContextCurrent(window);
			
			if (!create_info.vsync) { glfwSwapInterval(0); }

			// GLAD tries to load the context set by GLFW
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				throw std::runtime_error{ "Failed to initialize OpenGL context" };
			}

			// we define the viewport dimensions
			glViewport(0, 0, create_info.viewport_width, create_info.viewport_height);

			// we enable Z test
			glEnable(GL_DEPTH_TEST);

			// enable culling	
			glEnable(GL_CULL_FACE);

			// Init debug callbacks
			int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
			if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			{
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(debugCallback, NULL);
				glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
			}

			std::cout << "Window and context initialized!" << std::endl;
			std::cout << "Vendor:" << glGetString(GL_VENDOR) << std::endl;
			std::cout << "Renderer:" << glGetString(GL_RENDERER) << std::endl;
			std::cout << "Version:" << glGetString(GL_VERSION) << std::endl;

			return window;
		}
	};
}