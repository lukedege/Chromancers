#pragma once

#include <string>
#include <array>

#include <glad.h>
#include <stb_image.h>

#include <gsl/gsl>

#define DEFAULT_TEX_PATH "textures/default.png"

namespace engine::resources
{
	// Class for loading and managing textures 
	class Texture
	{
	public:
		// Simple class to hold information about a texture's format
		struct FormatInfo
		{
			GLenum internal_format;
			GLenum format;
			GLenum data_type;

			static FormatInfo sample_color_attachment_info()
			{
				return { GL_RGB, GL_RGB, GL_UNSIGNED_BYTE };
			}

			static FormatInfo sample_depth_attachment_info()
			{
				return {GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT};
			}
		};
		

	private:
		unsigned int _width;
		unsigned int _height;
		FormatInfo _format_info;

		GLuint _id;
	public:
		// Create a blank texture (useful for color attachments)
		Texture(unsigned int width = 512, unsigned int height = 512, Texture::FormatInfo tx_format_info = Texture::FormatInfo::sample_color_attachment_info()) :
			_id{ generate_texture() },
			_width      { width  },
			_height     { height },
			_format_info{ tx_format_info }
		{
			create_texture();
		}

		// Create a texture from a file
		Texture(const std::string& path, bool vflip_on_load = false) : 
			_id{ generate_texture() }
		{
			load_texture(path, vflip_on_load);
		}

		           Texture(const Texture& copy) = delete;
		Texture& operator=(const Texture& copy) = delete;

		Texture(Texture&& move) noexcept :
			_id         { move._id },
			_width      { move._width  },
			_height     { move._height },
			_format_info{ move._format_info }
		{
			// invalidate other's texture id since it has moved
			move._id = 0;
		};

		Texture& operator=(Texture&& move) noexcept
		{
			// Free up our resources to receive the new ones from the move
			dispose();

			if (move._id)
			{
				_id = move._id;
				_width = move._width;
				_height = move._height;
				_format_info = std::move(move._format_info);

				// invalidate other's texture id since it has moved
				move._id = 0;
			}
			else
			{
				_id = 0;
			}

			return *this;
		};

		~Texture()
		{
			dispose();
		}

		void load_texture(const std::string& path, bool vflip_on_load = false) noexcept
		{
			stbi_set_flip_vertically_on_load(vflip_on_load);

			unsigned char* image;

			int w, h, c;

			image = stbi_load(path.c_str(), &w, &h, &c, 0);

			if (image == nullptr)
			{
				// Load default texture
				std::cout << "Failed to load texture: " << stbi_failure_reason() << std::endl;
				image = stbi_load(DEFAULT_TEX_PATH, &w, &h, &c, 0);
			}

			_width    = gsl::narrow_cast<unsigned int>(w);
			_height   = gsl::narrow_cast<unsigned int>(h);

			GLenum format = choose_color_format(gsl::narrow_cast<unsigned int>(c));

			_format_info.format = format;
			_format_info.internal_format = format;
			_format_info.data_type = GL_UNSIGNED_BYTE;

			// bind
			glBindTexture(GL_TEXTURE_2D, _id);

			// create texture
			glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, _format_info.data_type, image);
			
			// generate mipmaps
			glGenerateMipmap(GL_TEXTURE_2D);

			// we set how to consider UVs outside [0,1] range
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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
			glBindTexture(GL_TEXTURE_2D, _id);
		}

		void unbind() const noexcept
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		unsigned int width ()                    const noexcept { return _width; }
		unsigned int height()                    const noexcept { return _height; }
		const Texture::FormatInfo& format_info() const noexcept { return _format_info; }
		int id()                                 const noexcept { return _id; }

		/*
		enum class WrapMode
		{
			REPEAT = GL_REPEAT,
			MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
			CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
			MIRROR_CLAMP_TO_EDGE = GL_MIRROR_CLAMP_TO_EDGE,
			CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
		};
		enum class MinFilter
		{
			NEAREST = GL_NEAREST,
			LINEAR = GL_LINEAR,
			NEAREST_WITH_MIPMAPS = GL_NEAREST_MIPMAP_NEAREST,
			LINEAR_WITH_MIPMAPS = GL_LINEAR_MIPMAP_LINEAR,
		};
		enum class MaxFilter
		{
			NEAREST = GL_NEAREST,
			LINEAR = GL_LINEAR,
		};

		void set_wrap_mode(WrapMode mode) const noexcept
		{
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(mode));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(mode));
			unbind();
		}

		void set_min_filtering(MinFilter filter) const noexcept
		{
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filter));
			unbind();
		}

		void set_max_filtering(MaxFilter filter) const noexcept
		{
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filter));
			unbind();
		}

		void set_border_color(const std::array<GLuint, 4>& borderColor)
		{
			bind();
			glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor.data());
			unbind();
		}

		void create_texture(GLenum internal_format, GLenum format, GLenum data_type, const void* data)
		{
			bind();
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, _width, _height, 0, format, data_type, data);
			unbind();
		}*/

		// extensible when needed, for now is enough
	private:
		// Generate a texture id from openGL
		GLuint generate_texture() const noexcept
		{
			GLuint texture_id;
			glGenTextures(1, &texture_id);
			
			return texture_id;
		}

		// Creates a blank texture given a texture id
		void create_texture()
		{
			glBindTexture(GL_TEXTURE_2D, _id);
			glTexImage2D(GL_TEXTURE_2D, 0, _format_info.internal_format, _width, _height, 0, _format_info.format, _format_info.data_type, nullptr);

			// Must be setup as default values 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void dispose() noexcept
		{
			glDeleteTextures(1, &_id);
		}

		// Helper function for choosing the right color format given the input image channels
		GLenum choose_color_format(unsigned int channels) const noexcept
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