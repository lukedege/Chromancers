#pragma once
#include <string>

namespace utils::strings
{
	// Erases the whole line at the specified string index (if valid)
	// TODO: when the index points at a "\n" character the line isnt deleted, to fix or give meaning to
	//       For now, this isn't a problem since it's only used in junction with find functions
	inline std::string eraseLineAt(const std::string source, const size_t pos)
	{
		std::string ret = source;
		if (pos < source.length())
		{
			size_t posNextNL = source.find("\n", pos);
			size_t posPrevNL = source.rfind("\n", pos);

			if (posPrevNL == std::string::npos) posPrevNL = 0;
			if (posNextNL == std::string::npos) posNextNL = source.length();

			ret.erase(posPrevNL, posNextNL - posPrevNL);
		}

		return ret;
	}

	// Erases first line found containing toErase string from source string
	inline std::string eraseLineContaining(const std::string source, const std::string toErase)
	{
		size_t posContent = source.find(toErase);
		std::string ret = eraseLineAt(source, posContent);

		return ret;
	}

	// Erases all lines containing toErase string
	inline std::string eraseAllLinesContaining(const std::string source, const std::string toErase)
	{
		std::string ret = source;
		size_t posContent = ret.find(toErase);
		while (posContent != std::string::npos)
		{
			ret = eraseLineAt(ret, posContent);
			posContent = ret.find(toErase);
		}

		return ret;
	}
}

namespace utils::graphics::opengl
{
	inline void setup_buffer_object(GLuint& buffer_object, GLenum target, size_t element_size, size_t element_amount, int bind_index, void* data)
	{
		glGenBuffers(1, &buffer_object);
		glBindBuffer(target, buffer_object);

		size_t alloc_size = element_size * element_amount;
		glBufferData(target, alloc_size, NULL, GL_DYNAMIC_DRAW); // allocate alloc_size bytes of memory
		glBindBufferBase(target, bind_index, buffer_object);

		if (data != 0)
			glBufferSubData(target, 0, alloc_size, data);        // fill buffer object with data

		glBindBuffer(target, 0);
	}

	inline void update_buffer_object(GLuint& buffer_object, GLenum target, size_t offset, size_t element_size, size_t element_amount, void* data)
	{
		glBindBuffer(target, buffer_object);
		glBufferSubData(target, offset, element_amount * element_size, data);
		glBindBuffer(target, 0);
	}
}