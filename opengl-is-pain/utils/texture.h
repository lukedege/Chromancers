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

		Texture(int width, int height, int channels) : 
			id{ generate_texture() },
			_width   { width    },
			_height  { height   },
			_channels{ channels }
		{}

		Texture(const std::string& path) : id{ generate_texture() }
		{
			load_texture(path);
		}

		~Texture()
		{
			dispose(); //TODO idk if this is the right thing to do looking at shader...
		}

		void load_texture(const std::string& path) noexcept
		{
			//stbi_set_flip_vertically_on_load(1);
			unsigned char* image;

			image = stbi_load(path.c_str(), &_width, &_height, &_channels, 0);

			if (image == nullptr)
			{
				std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
				return;
			}

			GLenum format = choose_color_format(_channels);

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
		// N.B. we are only binding the texture to the context and NOT using glActiveTexture because that activates a specific texture unit for the shader, and the texture unit id is 
		// used to sample in the shader, not the texture id!
		// thus for any texture we can always use the same texture unit(e.g. TEXTURE0); 
		// if we need more textures to sample in a shader (more sampler2ds) we activate different units, one for each texture. We do this in the material class
		void bind() const noexcept
		{
			glBindTexture(GL_TEXTURE_2D, id);
		}

		void unbind() const noexcept
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		
		int width ()     const noexcept { return _width   ; }
		int height()     const noexcept { return _height  ; }
		int n_channels() const noexcept { return _channels; }

		// extensible when needed, for now is enough
	private:
		GLuint generate_texture() const noexcept
		{
			GLuint texture_id;
			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);
			// setting default parameters for texture 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glBindTexture(GL_TEXTURE_2D, 0);
			return texture_id;
		}

		void dispose() noexcept
		{
			glDeleteTextures(1, &id);
		}


		GLenum choose_color_format(int channels) const noexcept
		{
			GLenum format;
			if (channels == 1)
				format = GL_RED;
			else if (channels == 3)
				format = GL_RGB;
			else if (channels == 4)
				format = GL_RGBA;
			else
			{
				std::cout << "Unrecognized format with: " << channels << " channels, defaulting to RGBA" << std::endl;
				format = GL_RGBA;
			}
			return format;
		}


	};
}