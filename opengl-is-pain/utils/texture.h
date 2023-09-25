#pragma once

#include <string>

#include <glad.h>
#include <stb_image.h>

namespace engine::resources
{
	class Texture
	{
	private:
		int _width;
		int _height;
		int _channels;

	public:
		GLuint id;

		Texture() : id{ generate_texture() } {}

		Texture(const std::string& path) : id{ generate_texture() }
		{
			load_texture(path);
		}

		void load_texture(const std::string& path)
		{
			//stbi_set_flip_vertically_on_load(1);
			unsigned char* image;

			image = stbi_load(path.c_str(), &_width, &_height, &_channels, 0);

			if (image == nullptr)
			{
				std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
				return;
			}

			GLenum format;
			if (_channels == 1)
				format = GL_RED;
			else if (_channels == 3)
				format = GL_RGB;
			else if (_channels == 4)
				format = GL_RGBA;
			else
			{
				std::cout << "Unrecognized format with: " << _channels << " channels" << std::endl;
				return;
			}

			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, format, _width, _height, 0, format, GL_UNSIGNED_BYTE, image);

			glGenerateMipmap(GL_TEXTURE_2D);
			// we set how to consider UVs outside [0,1] range
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			// we set the filtering for minification and magnification
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// we set the binding to 0 once we have finished
			glBindTexture(GL_TEXTURE_2D, 0);

			// we free the memory once we have created an OpenGL texture
			stbi_image_free(image);
		}

		// Bind to OpenGL context
		void bind()
		{
			glActiveTexture(GL_TEXTURE0 + id); // this should be correct as of: https://stackoverflow.com/questions/8866904/differences-and-relationship-between-glactivetexture-and-glbindtexture
			glBindTexture(GL_TEXTURE_2D, id);
		}

		void unbind()
		{
			glActiveTexture(GL_TEXTURE0 + id);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		int width ()     { return _width   ; }
		int height()     { return _height  ; }
		int n_channels() { return _channels; }

		// extensible when needed, for now is enough
	private:


		GLuint generate_texture()
		{
			GLuint texture_id;
			glGenTextures(1, &texture_id);
			return texture_id;
		}


	};
}