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
	public:
		GLuint program;

		Shader(const GLchar* vertPath, const GLchar* fragPath, const GLchar* geomPath = 0, std::vector<const GLchar*> utilPaths = {}, GLuint glMajor = 4, GLuint glMinor = 3, bool fromSpirvBinary = false) :
			program{ glCreateProgram() }, glMajorVersion{ glMajor }, glMinorVersion{ glMinor }
		{
			if (fromSpirvBinary)
				loadFromSpirV(vertPath, fragPath, geomPath, utilPaths);
			else
				loadFromText(vertPath, fragPath, geomPath, utilPaths);
		}

		// this is necessary since we dont want to delete the program involuntarily after a move (which would call the destructor)
		void del() const noexcept { glDeleteProgram(program); }

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
		void setBool (const std::string& name, bool value)                            const { glUniform1i(glGetUniformLocation(program, name.c_str()), (int)value); }
		void setInt  (const std::string& name, int value)                             const { glUniform1i(glGetUniformLocation(program, name.c_str()), value); }
		void setUint (const std::string& name, unsigned int value)                    const { glUniform1ui(glGetUniformLocation(program, name.c_str()), value); }
		void setFloat(const std::string& name, float value)                           const { glUniform1f(glGetUniformLocation(program, name.c_str()), value); }

		void setVec2 (const std::string& name, const GLfloat value[])                 const { glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec2 (const std::string& name, const glm::vec2& value)                const { glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec2 (const std::string& name, float x, float y)                      const { glUniform2f(glGetUniformLocation(program, name.c_str()), x, y); }
					 
		void setVec3 (const std::string& name, const GLfloat value[])                 const { glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec3 (const std::string& name, const glm::vec3& value)                const { glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec3 (const std::string& name, float x, float y, float z)             const { glUniform3f(glGetUniformLocation(program, name.c_str()), x, y, z); }
					 
		void setVec4 (const std::string& name, const GLfloat value[])                 const { glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, &value[0]); }
		void setVec4 (const std::string& name, const glm::vec4& value)                const { glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value)); }
		void setVec4 (const std::string& name, float x, float y, float z, float w)    const { glUniform4f(glGetUniformLocation(program, name.c_str()), x, y, z, w); }
					 
		void setMat2 (const std::string& name, const glm::mat2& mat)                  const { glUniformMatrix2fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }

		void setMat3 (const std::string& name, const glm::mat3& mat)                  const { glUniformMatrix3fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }
					 
		void setMat4 (const std::string& name, const glm::mat4& mat)                  const { glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat)); }
#pragma endregion 

	private:
		GLuint glMajorVersion;
		GLuint glMinorVersion;

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

			vertexShader = compileShader(vertSource, GL_VERTEX_SHADER, utilsSource);
			fragmentShader = compileShader(fragSource, GL_FRAGMENT_SHADER, utilsSource);

			glAttachShader(program, vertexShader);
			glAttachShader(program, fragmentShader);

			if (geomPath)
				{
				const std::string geomSource = loadSourceText(geomPath);
				geometryShader = compileShader(geomSource, GL_GEOMETRY_SHADER, utilsSource);
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

		void loadFromSpirV(const GLchar* vertPath, const GLchar* fragPath, const GLchar* geomPath, std::vector<const GLchar*> utilPaths = {})
		{
			//TODO from example code at https://www.khronos.org/opengl/wiki/SPIR-V
			// probably hard to do / not doable considering the shader "includes" done manually
			// UPDATE: actually glslc (spirv compiler) supports #include as long as included stuff doesnt have
			// "#version ..." in it, so its doable but will require some changes to loadfromtext
			// to keep both methods working
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

		GLuint compileShader(const std::string& shaderSource, GLenum shaderType, const std::string& utilsSource = "") const noexcept
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
			checkCompileErrors(shader);
			return shader;
		}

		void checkCompileErrors(GLuint shader) const noexcept
		{
			// Check for compile time errors TODO
			GLint success; GLchar infoLog[512];
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
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
	};
}