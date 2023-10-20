#pragma once

#include <glad.h>

#include "texture.h"
#include "window.h"

namespace
{
	using namespace engine::resources;
}

namespace utils::graphics::opengl
{
	class Framebuffer
	{
	public:

		Framebuffer(unsigned int width, unsigned int height, Texture::FormatInfo color_att_format_info, Texture::FormatInfo depth_att_format_info) :
			_id{ generate_framebuffer() },
			_width  { width  }, 
			_height { height },
			color_attachment { create_color_attachment(width, height, color_att_format_info) },
			depth_attachment { create_depth_attachment(width, height, depth_att_format_info) }
			//depth_buffer     { /*create_depth_buffer()*/} // you need this only when not using a depth attachment
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _id);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				utils::io::error("FRAMEBUFFER - Framebuffer is not complete!");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		           Framebuffer(const Framebuffer& copy) = delete;
		Framebuffer& operator=(const Framebuffer& copy) = delete;

		Framebuffer(Framebuffer&& move) :
			_id     { move._id },
			_width  { move._width  }, 
			_height { move._height },
			color_attachment { std::move(move.color_attachment) },
			depth_attachment { std::move(move.depth_attachment) }
		{
			move._id = 0;
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
				color_attachment = std::move(move.color_attachment);
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
		}

		void bind()
		{
			int viewport_size_data[4];
			glGetIntegerv(GL_VIEWPORT, viewport_size_data);
			old_width = viewport_size_data[2];
			old_height = viewport_size_data[3];

			glViewport(0, 0, _width, _height);
			glBindFramebuffer(GL_FRAMEBUFFER, _id);
		}

		void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, old_width, old_height);
		}

		Texture& get_color_attachment()
		{
			return color_attachment;
		}

		Texture& get_depth_attachment()
		{
			return depth_attachment;
		}

		unsigned int width () const noexcept { return _width;  }
		unsigned int height() const noexcept { return _height; }
		unsigned int id()     const noexcept { return _id; }
	private:
		GLuint _id;
		unsigned int _width, _height;
		unsigned int old_width, old_height; // stores the glviewport values before binding
		Texture color_attachment; // can/will be an array when supporting multiple color attachments, for now one is enough (and necessary for framebuffer completion)
		Texture depth_attachment;
		//GLuint  depth_buffer; // you need this only when not using a depth attachment

		GLuint generate_framebuffer()
		{
			GLuint framebuffer;
			glGenFramebuffers(1, &framebuffer);
			return framebuffer;
		}

		Texture create_color_attachment(unsigned int width, unsigned int height, Texture::FormatInfo color_att_format_info)
		{
			GLuint attachment_number = 0; // for when multiple attachments are implemented

			Texture new_color_attachment{ width, height, color_att_format_info };

			glBindFramebuffer(GL_FRAMEBUFFER, _id);
			glBindTexture(GL_TEXTURE_2D, new_color_attachment.id());

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_number, GL_TEXTURE_2D, new_color_attachment.id(), 0);
			
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_color_attachment;
		}

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
		
		GLuint create_depth_buffer()
		{
			GLuint depth_buffer;
			glGenRenderbuffers(1, &depth_buffer);

			glBindFramebuffer(GL_FRAMEBUFFER, _id);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
			
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height); // maybe also only depth
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
			
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return depth_buffer;
		}

		void dispose()
		{
			glDeleteFramebuffers(1, &_id);
			//glDeleteRenderbuffers(1, &depth_buffer);
		}
	};
}