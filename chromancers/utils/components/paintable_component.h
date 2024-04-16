#pragma once

#include <bitmap_image.hpp>

#include "../component.h"
#include "../transform.h"
#include "../shader.h"
#include "../framebuffer.h"

namespace engine::components
{
	// 1) Renderizzare la scena in paintspace su framebuffer
	// 2) Data la direzione di collisione, modifica nello shader la paint_mask con i dati UV di ogni fragment
	// 3) Risalva l'immagine e usala come texture nello scene draw finale
	class PaintableComponent : public Component
	{
		using Shader = engine::resources::Shader;
		using Texture = engine::resources::Texture;
		using Framebuffer = utils::graphics::opengl::Framebuffer;

	public:
		constexpr static auto COMPONENT_ID = 2;

	private:
		Texture paint_map;

		Shader* painter_shader;
		Texture* splat_tex; // tex to apply to impact
		Texture* paint_normal_map; // tex to apply to impact
		Framebuffer paint_fbo; // this framebuffer simply stands to avoid polluting the main rendering framebuffer with drawcalls

	public:

		PaintableComponent(Entity& parent, Shader& painter_shader, unsigned int paintmap_width, unsigned int paintmap_height, Texture* splat_tex, Texture* paint_normal_map = nullptr) :
			Component(parent),
			paint_map { paintmap_width, paintmap_height, {GL_RGBA8, GL_RGBA, GL_FLOAT} },
			painter_shader{ &painter_shader },
			splat_tex{ splat_tex },
			paint_normal_map{ paint_normal_map } 
		{
			std::vector<GLfloat> pixels(paintmap_width * paintmap_height * 4, 0.f);
			
			const auto& pmap_format = paint_map.format_info();

			paint_map.bind();
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintmap_width, paintmap_height, pmap_format.format, pmap_format.data_type, pixels.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			paint_map.unbind();

			parent.material->detail_diffuse_map = &paint_map;
			parent.material->detail_normal_map = paint_normal_map;
		}

		void init()
		{

		}

		void update(float delta_time)
		{

		}

		int type()
		{
			return COMPONENT_ID;
		}

		void update_paintmap(const glm::mat4& paintspace_matrix, const glm::vec3& paint_direction, const glm::vec4& paint_color)
		{
			paint_fbo.bind();
			painter_shader->bind();
			painter_shader->setMat4("paintSpaceMatrix", paintspace_matrix);
			painter_shader->setVec3("paintBallDirection", paint_direction);
			painter_shader->setVec4("paintBallColor", paint_color);
			
			// setup splat mask
			glActiveTexture(GL_TEXTURE0);
			splat_tex->bind();
			painter_shader->setInt("splat_mask", 0);

			// paint map
			painter_shader->setInt("paintmap_size", paint_map.width());
			glBindImageTexture(1, paint_map.id(), 0, GL_FALSE, 0, GL_READ_WRITE, paint_map.format_info().internal_format);
			
			//glClear(GL_DEPTH_BUFFER_BIT);
			parent->custom_draw(*painter_shader);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			paint_fbo.unbind();
		}

	private:
		// TODO temp
		void ExportMaskTexture(Texture& map_texture, unsigned int width, unsigned int height, std::string filename)
		{
			// Image Writing
			std::unique_ptr<GLubyte[]> imageData = std::make_unique<GLubyte[]>(width * height * 4);
			glBindTexture(GL_TEXTURE_2D, map_texture.id());
			glGetTexImage(GL_TEXTURE_2D, 0, map_texture.format_info().format, GL_UNSIGNED_BYTE, imageData.get());

			bitmap_image image(width, height);
			GLubyte* current_pixel = imageData.get();
			for (unsigned int y = 0; y < width; y++)
			{
				for (unsigned int x = 0; x < height; x++, current_pixel++) // skip alpha
				{
					// BGR
					GLubyte blue  = *(current_pixel++);
					GLubyte green = *(current_pixel++);
					GLubyte red   = *(current_pixel++);
					image.set_pixel(x, y, red, green, blue);
				}
			}

			image.save_image(filename);
			glBindTexture(GL_TEXTURE_2D, 0);
			if(true){}
		}

	};
}