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
		Texture* stain_tex; // tex to apply to impact
		

	public:

		PaintableComponent(Entity& parent, Shader& painter_shader, Texture& stain_tex, unsigned int paintmask_width, unsigned int paintmask_height) :
			Component(parent),
			paint_mask{ paintmask_width, paintmask_height, {GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE} },
			painter_shader{ &painter_shader },
			stain_tex{ &stain_tex }
		{
			paint_mask.bind();
			
			const auto& tx_format = paint_mask.format_info();

			std::vector<GLubyte> pixels(paintmask_width * paintmask_height, (GLubyte)UINT_MAX);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintmask_width, paintmask_height, tx_format.format, tx_format.data_type, pixels.data());

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			paint_mask.unbind();

			parent.material->detail_map = &paint_mask;
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
			
			// setup stain
			glActiveTexture(GL_TEXTURE0);
			stain_tex->bind();
			painter_shader->setInt("stainTex", 0);
			
			// paint mask
			painter_shader->setInt("paintmap_size", 512);
			glBindImageTexture(3, paint_mask.id(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);
			
			//glClear(GL_DEPTH_BUFFER_BIT);
			parent->custom_draw(*painter_shader);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//ExportMaskTexture(paint_mask.id(), 512, 512, "test.bmp");
		}
		
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
	private:

		
	};
}