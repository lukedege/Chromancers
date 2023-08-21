#pragma once
/*
   Shader class
   - loading Shader source code, Shader Program creation
*/

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"

namespace utils::graphics::opengl
{

	class Shader
	{
	private:
		GLuint glMajorVersion;
		GLuint glMinorVersion;
		std::string vertPath, fragPath;
	public:
		GLuint program;

		Shader(const GLchar* vertPath, const GLchar* fragPath, GLuint glMajor, GLuint glMinor, const GLchar* geomPath = 0, std::vector<const GLchar*> utilPaths = {}) :
			program{ glCreateProgram() }, glMajorVersion{ glMajor }, glMinorVersion{ glMinor },
			vertPath{ vertPath }, fragPath{ fragPath }
		{
			loadFromText(vertPath, fragPath, geomPath, utilPaths);
		}
		
		Shader(const GLchar* vertSpirvPath, const GLchar* fragSpirvPath, const GLchar* geomSpirvPath = 0) :
			program{ glCreateProgram() }, glMajorVersion{ 4 }, glMinorVersion{ 6 } // SpirV is core from version 4.6
		{
			loadFromSpirV(vertSpirvPath, fragSpirvPath, geomSpirvPath);
		}

		// this is necessary since we dont want to delete the program involuntarily after a move (which would call the destructor)
		void dispose() const noexcept { glDeleteProgram(program); }

		void use() const noexcept { glUseProgram(program); }

		std::vector<std::string> findSubroutines(GLenum shaderType)
		{
			std::vector<std::string> ret;

			// Sub = subroutines
			int maxSubsAmount = 0, maxSubsUniforms = 0, activeSubUniforms = 0;
			GLchar activeSubUniformName[256];
			int nameLength = 0, compatibleSubsAmount = 0;

			// global parameters about the Subroutines parameters of the system
			glGetIntegerv(GL_MAX_SUBROUTINES, &maxSubsAmount);
			glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubsUniforms);
			std::cout << "Max Subroutines:" << maxSubsAmount << " - Max Subroutine Uniforms:" << maxSubsUniforms << std::endl;

			// get the number of Subroutine uniforms for the kind of shader used
			glGetProgramStageiv(program, shaderType, GL_ACTIVE_SUBROUTINE_UNIFORMS, &activeSubUniforms);

			// print info for every Subroutine uniform
			for (int i = 0; i < activeSubUniforms; i++) {

				// get the name of the Subroutine uniform (in this example, we have only one)
				glGetActiveSubroutineUniformName(program, shaderType, i, 256, &nameLength, activeSubUniformName);
				// print index and name of the Subroutine uniform
				std::cout << "Subroutine Uniform: " << i << " - name: " << activeSubUniformName << std::endl;

				// get the number of subroutines for the current active subroutine uniform
				glGetActiveSubroutineUniformiv(program, shaderType, i, GL_NUM_COMPATIBLE_SUBROUTINES, &compatibleSubsAmount);

				// get the indices of the active subroutines info and write into the array 
				// TODO check why there's a mismatch in indices
				std::cout << "glGetSubroutineIndex call" << std::endl;
				std::cout << "\t" << glGetSubroutineIndex(program, shaderType, "Lambert") << " - " << "Lambert" << std::endl;
				std::cout << "\t" << glGetSubroutineIndex(program, shaderType, "Phong") << " - " << "Phong" << std::endl;
				std::cout << "\t" << glGetSubroutineIndex(program, shaderType, "BlinnPhong") << " - " << "BlinnPhong" << std::endl;
				std::cout << "\t" << glGetSubroutineIndex(program, shaderType, "GGX") << " - " << "GGX" << std::endl;

				std::vector<int> compatibleSubs(compatibleSubsAmount);
				glGetActiveSubroutineUniformiv(program, shaderType, i, GL_COMPATIBLE_SUBROUTINES, compatibleSubs.data());
				std::cout << "Compatible Subroutines:" << std::endl;

				// for each index, get the name of the subroutines, print info, and save the name in the shaders vector
				std::cout << "glGetActiveSubroutineName call on glGetActiveSubroutineUniformiv returned array values" << std::endl;
				for (int j = 0; j < compatibleSubsAmount; j++) {
					
					glGetActiveSubroutineName(program, shaderType, compatibleSubs[j], 256, &nameLength, activeSubUniformName);
					std::cout << "\t" << compatibleSubs[j] << " - " << activeSubUniformName << "\n";
					ret.push_back(activeSubUniformName);
				}
				std::cout << std::endl;
			}

			return ret;
		}

		GLuint getSubroutineIndex(GLenum shaderType, const char* subroutineName)
		{
			return glGetSubroutineIndex(program, shaderType, subroutineName);
		}

#pragma region utility_uniform_functions
		void setInt  (const std::string& name, int value)                             const { glUniform1i (glGetUniformLocation(program, name.c_str()), static_cast<GLint>(value)); }
		void setBool (const std::string& name, bool value)                            const { glUniform1i (glGetUniformLocation(program, name.c_str()), static_cast<GLint>(value)); }
		void setUint (const std::string& name, unsigned int value)                    const { glUniform1ui(glGetUniformLocation(program, name.c_str()), static_cast<GLuint>(value)); }
		void setFloat(const std::string& name, float value)                           const { glUniform1f (glGetUniformLocation(program, name.c_str()), static_cast<GLfloat>(value)); }

		void setVec2 (const std::string& name, const GLfloat value[])                 const { glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec2 (const std::string& name, const glm::vec2& value)                const { glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec2 (const std::string& name, float x, float y)                      const { glUniform2f (glGetUniformLocation(program, name.c_str()), static_cast<GLfloat>(x), static_cast<GLfloat>(y)); }
					 
		void setVec3 (const std::string& name, const GLfloat value[])                 const { glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec3 (const std::string& name, const glm::vec3& value)                const { glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec3 (const std::string& name, float x, float y, float z)             const { glUniform3f (glGetUniformLocation(program, name.c_str()), static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z)); }
					 
		void setVec4 (const std::string& name, const GLfloat value[])                 const { glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec4 (const std::string& name, const glm::vec4& value)                const { glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec4 (const std::string& name, float x, float y, float z, float w)    const { glUniform4f (glGetUniformLocation(program, name.c_str()), static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z), static_cast<GLfloat>(w)); }
					 
		void setMat2 (const std::string& name, const glm::mat2& mat)                  const { glUniformMatrix2fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }

		void setMat3 (const std::string& name, const glm::mat3& mat)                  const { glUniformMatrix3fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }
					 
		void setMat4 (const std::string& name, const glm::mat4& mat)                  const { glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }
#pragma endregion 

	private:	
		void checkCompileErrors(GLuint shader, GLenum shaderType) const noexcept
		{
			// Check for compile time errors TODO
			GLint success; GLchar infoLog[512];
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 512, NULL, infoLog);
				std::string shader_type;
				if (shaderType == GL_VERTEX_SHADER)        shader_type = "VERTEX";
				else if (shaderType == GL_FRAGMENT_SHADER) shader_type = "FRAGMENT";
				else if (shaderType == GL_GEOMETRY_SHADER) shader_type = "GEOMETRY";
				std::cout << "ERROR::" << shader_type << " SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
			}
		}

		void checkLinkingErrors() const noexcept
		{
			GLint success; GLchar infoLog[512];
			glGetProgramiv(program, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
		}

#pragma region text
		void loadFromText(const GLchar* vertPath, const GLchar* fragPath, const GLchar* geomPath, std::vector<const GLchar*> utilPaths = {})
		{
			GLuint vertexShader, fragmentShader, geometryShader;

			const std::string vertSource = loadSourceText(vertPath);
			const std::string fragSource = loadSourceText(fragPath);

			std::string utilsSource;

			for (const GLchar* utilPath : utilPaths)
			{
				utilsSource += loadSourceText(utilPath) + "\n";
			}

			vertexShader = compileShaderText(vertSource, GL_VERTEX_SHADER, utilsSource);
			fragmentShader = compileShaderText(fragSource, GL_FRAGMENT_SHADER, utilsSource);

			glAttachShader(program, vertexShader);
			glAttachShader(program, fragmentShader);

			if (geomPath)
			{
				const std::string geomSource = loadSourceText(geomPath);
				geometryShader = compileShaderText(geomSource, GL_GEOMETRY_SHADER, utilsSource);
				glAttachShader(program, geometryShader);
			}

			try {
				glLinkProgram(program);
			}
			catch (std::exception e) { auto x = glGetError(); checkLinkingErrors(); std::cout << e.what(); }
			checkLinkingErrors();

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			if (geomPath) { glDeleteShader(geometryShader); }
		}

		const std::string loadSourceText(const GLchar* sourcePath) const noexcept
		{
			std::string         sourceText;
			std::ifstream       sourceFile;
			std::stringstream sourceStream;

			sourceFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

			try
			{
				sourceFile.open(sourcePath);
				sourceStream << sourceFile.rdbuf();
				sourceFile.close();
				sourceText = sourceStream.str();
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}

			// Pre-process for merged text compilation 
			// Find and remove #version (will be added later) and eventual #include 
			sourceText = utils::strings::eraseLineContaining(sourceText, "#version");
			sourceText = utils::strings::eraseAllLinesContaining(sourceText, "#include");

			return sourceText;
		}

		GLuint compileShaderText(const std::string& shaderSource, GLenum shaderType, const std::string& utilsSource = "") const noexcept
		{
			std::string mergedSource = "";
			// Prepending version
			mergedSource += "#version " + std::to_string(glMajorVersion) + std::to_string(glMinorVersion) + "0 core\n";

			// Prepending utils
			mergedSource += utilsSource + shaderSource;

			const std::string finalSource{ mergedSource };

			// Shader creation
			GLuint shader = glCreateShader(shaderType);
			const GLchar* c = finalSource.c_str(); // as of documentation, c_str return a null terminated string
			
			glShaderSource(shader, 1, &c, NULL);   // as of documentation, glShaderSource is supposed to copy the content of the c string so we can trash it afterwards
			glCompileShader(shader);
			checkCompileErrors(shader, shaderType);
			return shader;
		}
#pragma endregion

#pragma region spirv
		void loadFromSpirV(const GLchar* vertPath, const GLchar* fragPath, const GLchar* geomPath, std::vector<const GLchar*> utilPaths = {})
		{
			//TODO from example code at https://www.khronos.org/opengl/wiki/SPIR-V
			GLuint vertexShader, fragmentShader;

			// Read our shaders into the appropriate buffers
			std::vector<char> vertexSpirv = loadSourceSpirV(vertPath);// Get SPIR-V for vertex shader.
			std::vector<char> fragmentSpirv = loadSourceSpirV(fragPath);// Get SPIR-V for fragment shader.

			fragmentShader = compileShaderSpirv(fragmentSpirv, GL_FRAGMENT_SHADER);
			vertexShader = compileShaderSpirv(vertexSpirv, GL_VERTEX_SHADER);


			// Vertex and fragment shaders are successfully compiled.
			// Now time to link them together into a program.

			// Attach our shaders to our program
			glAttachShader(program, vertexShader);
			glAttachShader(program, fragmentShader);

			// Link our program
			try {
				glLinkProgram(program);
			}
			catch (std::exception e) { auto x = glGetError(); checkLinkingErrors(); std::cout << e.what(); }
			checkLinkingErrors();

			// Always detach shaders after a successful link.
			glDetachShader(program, vertexShader); // delete or detach??
			glDetachShader(program, fragmentShader);
		}

		std::vector<char> loadSourceSpirV(const GLchar* spirvPath)
		{
			std::ifstream shaderFile;
			shaderFile.open(spirvPath, std::ios::binary | std::ios::ate);
			if (shaderFile.is_open())
			{
				size_t size = shaderFile.tellg();
				char* bin = new char[size];
				shaderFile.seekg(0, std::ios::beg);
				shaderFile.read(bin, size);
				std::vector<char> ret{ bin, bin + size };
				delete[] bin;
				return ret;
			}
			return {};
		}

		GLuint compileShaderSpirv(const std::vector<char> shaderSpirv, GLenum shaderType)
		{
			// Create an empty shader handle
			GLuint shader = glCreateShader(shaderType);

			// Apply the shader SPIR-V to the shader object.
			glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderSpirv.data(), static_cast<GLsizei>(shaderSpirv.size()));

			// Specialize the shader.
			//std::string entrypoint = "main"; // Get VS entry point name
			glSpecializeShader(shader, "main", 0, nullptr, nullptr);

			// Specialization is equivalent to compilation.
			checkCompileErrors(shader, shaderType);

			return shader;
		}
#pragma endregion
	};
}