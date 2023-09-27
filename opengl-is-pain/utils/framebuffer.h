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
			depth_buffer     { create_depth_buffer()     }
		{
			glBindFramebuffer(GL_FRAMEBUFFER, id);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		~Framebuffer()
		{
			dispose();
		}

		void bind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, id);
			//color_attachment.bind();
			//depth_attachment.bind();
			//glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		}

		void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//color_attachment.unbind();
			//depth_attachment.unbind();
			//glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}

		const Texture& get_color_attachment()
		{
			return color_attachment;
		}

		int width () const noexcept { return _width;  }
		int height() const noexcept { return _height; }

	private:
		int _width, _height;
		Texture color_attachment; // can/will be an array when supporting multiple color attachments, for now one is enough
		Texture depth_attachment;
		GLuint  depth_buffer;

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
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_number, new_color_attachment.id, 0);
			
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return new_color_attachment;
		}

		Texture create_depth_attachment()
		{
			Texture new_depth_attachment{ _width, _height, 1 };

			glBindFramebuffer(GL_FRAMEBUFFER, id);
			glBindTexture(GL_TEXTURE_2D, new_depth_attachment.id);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, new_depth_attachment.id, 0);

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