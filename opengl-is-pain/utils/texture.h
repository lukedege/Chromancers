#pragma once

#include <string>

#include <glad.h>
#include <stb_image.h>

namespace utils::graphics::opengl
{
	class Texture
	{
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
			int w, h, channels;
			unsigned char* image;

			image = stbi_load(path.c_str(), &w, &h, &channels, 0);

			if (image == nullptr)
			{
				std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
				return;
			}

			GLenum format;
			if (channels == 1)
				format = GL_RED;
			else if (channels == 3)
				format = GL_RGB;
			else if (channels == 4)
				format = GL_RGBA;
			else
			{
				std::cout << "Unrecognized format with: " << channels << " channels" << std::endl;
				return;
			}

			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, image);

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

		void activate()
		{
			glActiveTexture(GL_TEXTURE0 + id); // this should be correct as of: https://stackoverflow.com/questions/8866904/differences-and-relationship-between-glactivetexture-and-glbindtexture
			glBindTexture(GL_TEXTURE_2D, id);
		}

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