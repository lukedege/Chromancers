#pragma once

#include <glad.h>
#include <array>
#include <set>

#include "texture.h"
#include "window.h"

namespace utils::graphics::opengl
{
	// Class wrapping the OpenGL framebuffer
	class BasicFramebuffer
	{
	protected:
		GLuint _id;
		unsigned int _width, _height;
		unsigned int old_width, old_height; // stores the glviewport values before binding

	public:
		BasicFramebuffer(unsigned int width = 512, unsigned int height = 512) :
			_id{ generate_framebuffer() },
			_width  { width  }, 
			_height { height }
		{}
		
		           BasicFramebuffer(const BasicFramebuffer& copy) = delete;
		BasicFramebuffer& operator=(const BasicFramebuffer& copy) = delete;

		BasicFramebuffer(BasicFramebuffer&& move) :
			_id     { move._id },
			_width  { move._width  }, 
			_height { move._height }
		{
			move._id = 0;
		}

		BasicFramebuffer& operator=(BasicFramebuffer&& move)
		{
			dispose();

			// Check if it exists
			if (move._id)
			{
				_id = move._id;
				_width  = move._width ; 
				_height = move._height;

				move._id = 0;
			}
			else
			{
				_id = 0;
			}

			return *this;
		}

		~BasicFramebuffer()
		{
			dispose();
		}

		// Bind this framebuffer, setting the viewport extent to match the framebuffers'
		void bind()
		{
			int viewport_size_data[4];
			glGetIntegerv(GL_VIEWPORT, viewport_size_data);
			old_width = viewport_size_data[2];
			old_height = viewport_size_data[3];

			glViewport(0, 0, _width, _height);
			glBindFramebuffer(GL_FRAMEBUFFER, _id);
		}

		// Unbind this framebuffer, resetting the viewport to previous values
		void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, old_width, old_height);
		}

		// N.B. viewport restoring functionality from bind/unbind is guaranteed
		// only if the methods are called in a coupled way
		// e.g. bind = "(", unbind = ")"
		// ()[](), ([]), [()()] are okay configurations
		// ([)] is an example of not okay configuration

		unsigned int width () const noexcept { return _width;  }
		unsigned int height() const noexcept { return _height; }
		unsigned int id()     const noexcept { return _id; }

	protected:
		GLuint generate_framebuffer()
		{
			GLuint framebuffer;
			glGenFramebuffers(1, &framebuffer);
			return framebuffer;
		}

		void dispose()
		{
			glDeleteFramebuffers(1, &_id);
		}
	};

	// Class extending the basic framebuffer functionalities by keeping information about its own attachments (color and depth)
	class Framebuffer : public BasicFramebuffer
	{
		using Texture = engine::resources::Texture;

		unsigned int color_attachment_base_index = GL_COLOR_ATTACHMENT0;
		std::array<Texture, 8> color_attachments; // Array of color attachment textures; minimum value is 8 for OpenGL 3.x+ spec, GL macros goes up to 32 
		std::set<unsigned int> active_color_attachments; // Set of active (manually set and initialized) color attachments
		Texture depth_attachment;

	public:
		Framebuffer(unsigned int width = 512, unsigned int height = 512, 
			Texture::FormatInfo primary_color_att_format_info = Texture::FormatInfo::sample_color_attachment_info(), 
			Texture::FormatInfo depth_att_format_info = Texture::FormatInfo::sample_depth_attachment_info()) :
			BasicFramebuffer(width, height),
			color_attachments{ create_color_attachment(0, width, height, primary_color_att_format_info) },
			active_color_attachments { color_attachment_base_index },
			depth_attachment { create_depth_attachment(width, height, depth_att_format_info) }
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _id);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				utils::io::error("FRAMEBUFFER - Framebuffer is not complete!");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		           Framebuffer(const Framebuffer& copy) = delete;
		Framebuffer& operator=(const Framebuffer& copy) = delete;

		Framebuffer(Framebuffer&& move) :
			BasicFramebuffer(std::move(move)),
			color_attachments { std::move(move.color_attachments) },
			depth_attachment { std::move(move.depth_attachment) }
		{
			//move._id = 0; // should be done by parent already
		}

		Framebuffer& operator=(Framebuffer&& move)
		{
			dispose();

			// Check if it exists
			if (move._id)
			{
				_id = move._id;
				_width  = move._width ; 
				_height = move._height;
				color_attachments = std::move(move.color_attachments);
				depth_attachment = std::move(move.depth_attachment);

				move._id = 0;
			}
			else
			{
				_id = 0;
			}

			return *this;
		}

		~Framebuffer()
		{
			dispose();
			// attachment textures will go out of scope and delete themselves automatically
		}

		// Bind this framebuffer, setting the attachments to be available for Multiple Render Targets shader techniques
		void bind()
		{
			BasicFramebuffer::bind();

			std::vector<unsigned int> attachments{active_color_attachments.begin(), active_color_attachments.end()};
			glDrawBuffers(gsl::narrow<GLsizei>(attachments.size()), attachments.data());
		}

		// Return the i-th color attachment
		Texture& get_color_attachment(unsigned int index = 0)
		{
			if (!active_color_attachments.count(color_attachment_base_index + index))
				utils::io::warn("FRAMEBUFFER - Color attachment at ", index, " wasn't properly set!");
			return color_attachments[index];
		}

		// Set the i-th color attachment with a new texture with the given properties
		Texture& set_color_attachment(unsigned int index, Texture::FormatInfo color_att_format_info)
		{
			unsigned int idx = index;
			if (idx > color_attachments.size())
			{
				utils::io::error("FRAMEBUFFER - Attachment index out of bounds, defaulting to zero");
				idx = 0;
			}

			color_attachments[idx] = create_color_attachment(idx, _width, _height, color_att_format_info);
			active_color_attachments.insert(color_attachment_base_index + idx);
			return color_attachments[idx];
		}

		Texture& get_depth_attachment()
		{
			return depth_attachment;
		}
	private:
		// Create a color attachment texture with the given properties
		Texture create_color_attachment(GLuint attachment_index, unsigned int width, unsigned int height, Texture::FormatInfo color_att_format_info)
		{
			Texture new_color_attachment{ width, height, color_att_format_info };

			glBindFramebuffer(GL_FRAMEBUFFER, _id);
			glBindTexture(GL_TEXTURE_2D, new_color_attachment.id());

			glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachment_base_index + attachment_index, GL_TEXTURE_2D, new_color_attachment.id(), 0);
			
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_color_attachment;
		}

		// Create a depth attachment texture with the given properties
		Texture create_depth_attachment(unsigned int width, unsigned int height, Texture::FormatInfo depth_att_format_info)
		{
			Texture new_depth_attachment{ width, height, depth_att_format_info };

			glBindFramebuffer(GL_FRAMEBUFFER, _id);
			glBindTexture(GL_TEXTURE_2D, new_depth_attachment.id());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, new_depth_attachment.id(), 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_depth_attachment;
		}
	};
}