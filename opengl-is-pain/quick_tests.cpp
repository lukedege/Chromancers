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

// utils libraries
#include "utils/shader.h"
#include "utils/model.h "

#include "utils/scene/camera.h"
#include "utils/scene/entity.h"
#include "utils/scene/light.h "

#include "utils/window.h"

bool loadBinaryShader(const char* fileName, GLuint stage, GLuint binaryFormat, GLuint& shader)
{
	std::ifstream shaderFile;
	shaderFile.open(fileName, std::ios::binary | std::ios::ate);
	if (shaderFile.is_open())
	{
		size_t size = shaderFile.tellg();
		shaderFile.seekg(0, std::ios::beg);
		char* bin = new char[size];
		shaderFile.read(bin, size);

		GLint status;
		shader = glCreateShader(stage);									// Create a new shader
		glShaderBinary(1, &shader, binaryFormat, bin, size);			// Load the binary shader file
		glSpecializeShader(shader, "main", 0, nullptr, nullptr);		// Set main entry point (required, no specialization used in this example)
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);				// Check compilation status
		return status;
	}
	else
	{
		std::cerr << "Could not open \"" << fileName << "\"" << std::endl;
		return false;
	}
}

int mainz()
{
	namespace ugl = utils::graphics::opengl;

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

	std::cout << "Vendor:" << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer:" << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version:" << glGetString(GL_VERSION) << std::endl;


	GLuint vs;
	loadBinaryShader("shaders/bvert.spv", GL_VERTEX_SHADER, GL_SHADER_BINARY_FORMAT_SPIR_V, vs);


	return 0;
}