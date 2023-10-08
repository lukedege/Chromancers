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
		GLuint id;

		Framebuffer(int width, int height) :
			id{ generate_framebuffer() },
			_width  { width  }, 
			_height { height },
			color_attachment { create_color_attachment() },
			depth_attachment { create_depth_attachment() },
			depth_buffer     { /*create_depth_buffer()*/}
		{
			glBindFramebuffer(GL_FRAMEBUFFER, id);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				utils::io::error("FRAMEBUFFER - Framebuffer is not complete!");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
			glBindFramebuffer(GL_FRAMEBUFFER, id);
		}

		void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, old_width, old_height);
		}

		const Texture& get_color_attachment()
		{
			return color_attachment;
		}

		const Texture& get_depth_attachment()
		{
			return depth_attachment;
		}

		int width () const noexcept { return _width;  }
		int height() const noexcept { return _height; }

	private:
		int _width, _height;
		int old_width, old_height; // stores the glviewport values before binding
		Texture color_attachment; // can/will be an array when supporting multiple color attachments, for now one is enough
		Texture depth_attachment;
		GLuint  depth_buffer; // you need this only when not using a depth attachment

		GLuint generate_framebuffer()
		{
			GLuint framebuffer;
			glGenFramebuffers(1, &framebuffer);
			return framebuffer;
		}

		Texture create_color_attachment()
		{
			GLenum format = GL_RGB;
			GLuint attachment_number = 0; // for when multiple attachments are implemented

			Texture new_color_attachment{ _width, _height, 3 };

			glBindFramebuffer(GL_FRAMEBUFFER, id);
			glBindTexture(GL_TEXTURE_2D, new_color_attachment.id);

			glTexImage2D(GL_TEXTURE_2D, 0, format, _width, _height, 0, format, GL_UNSIGNED_BYTE, nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_number, GL_TEXTURE_2D, new_color_attachment.id, 0);
			
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_color_attachment;
		}

		Texture create_depth_attachment()
		{
			Texture new_depth_attachment{ _width, _height, 1 };

			glBindFramebuffer(GL_FRAMEBUFFER, id);
			glBindTexture(GL_TEXTURE_2D, new_depth_attachment.id);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, new_depth_attachment.id, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_depth_attachment;
		}
		
		GLuint create_depth_buffer()
		{
			GLuint depth_buffer;
			glGenRenderbuffers(1, &depth_buffer);

			glBindFramebuffer(GL_FRAMEBUFFER, id);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
			
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height); // maybe also only depth
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
			
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return depth_buffer;
		}

		void dispose()
		{
			glDeleteFramebuffers(1, &id);
			glDeleteRenderbuffers(1, &depth_buffer);
		}
	};
}