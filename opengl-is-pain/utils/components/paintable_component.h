#pragma once

#include <bitmap_image.hpp>

#include "../component.h"
#include "../transform.h"
#include "../shader.h"
#include "../framebuffer.h"

// unnamed namespace will keep this namespace declaration for this file only, even if included
namespace
{
	using namespace engine::scene;
	using namespace engine::resources;
	using namespace utils::graphics::opengl;
}

namespace engine::components
{
	// 1) Renderizzare la scena in paintspace su framebuffer
	// 2) Data la direzione di collisione, modifica nello shader la paint_mask con i dati UV di ogni fragment
	// 3) Risalva l'immagine e usala come texture nello scene draw finale
	class PaintableComponent : public Component
	{
	public:
		constexpr static auto COMPONENT_ID = 2;

	private:
		Texture paint_mask;

		Shader* painter_shader;
		Texture* splat_tex; // tex to apply to impact
		Texture* paint_normal_map; // tex to apply to impact

	public:

		PaintableComponent(Entity& parent, Shader& painter_shader, unsigned int paintmask_width, unsigned int paintmask_height, Texture* splat_tex, Texture* paint_normal_map = nullptr) :
			Component(parent),
			paint_mask{ paintmask_width, paintmask_height, {GL_RGBA32F, GL_RGBA, GL_FLOAT } },
			painter_shader{ &painter_shader },
			splat_tex{ splat_tex },
			paint_normal_map{ paint_normal_map } 
		{
			paint_mask.bind();
			
			const auto& tx_format = paint_mask.format_info();
			
			std::vector<GLfloat> pixels(paintmask_width * paintmask_height * 4, 0.f);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintmask_width, paintmask_height, tx_format.format, tx_format.data_type, pixels.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			paint_mask.unbind();

			parent.material->detail_diffuse_map = &paint_mask;
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
			painter_shader->bind();
			painter_shader->setMat4("paintSpaceMatrix", paintspace_matrix);
			painter_shader->setVec3("paintBallDirection", paint_direction);
			painter_shader->setVec4("paintBallColor", paint_color);
			
			// setup splat mask
			glActiveTexture(GL_TEXTURE0);
			splat_tex->bind();
			painter_shader->setInt("splat_mask", 0);
			
			// paint mask
			painter_shader->setInt("paintmap_size", 512);
			glBindImageTexture(3, paint_mask.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, paint_mask.format_info().internal_format);
			
			//glClear(GL_DEPTH_BUFFER_BIT);
			parent->custom_draw(*painter_shader);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//ExportMaskTexture(paint_mask.id(), 512, 512, "test.bmp");
		}

	private:
		// TODO temp
		void ExportMaskTexture(GLint mask_texture, unsigned int width, unsigned int height, std::string filename)
		{
			// Image Writing
			std::unique_ptr<GLubyte[]> imageData = std::make_unique<GLubyte[]>(width * height);
			glBindTexture(GL_TEXTURE_2D, mask_texture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, imageData.get());

			bitmap_image image(width, height);
			GLubyte* current_pixel = imageData.get();
			for (unsigned int y = 0; y < width; y++)
			{
				for (unsigned int x = 0; x < height; x++, current_pixel++)
				{
					image.set_pixel(x, y, *(current_pixel), *(current_pixel), *(current_pixel));
				}
			}

			image.save_image(filename);
			glBindTexture(GL_TEXTURE_2D, 0);
			if(true){}
		}
	};
}