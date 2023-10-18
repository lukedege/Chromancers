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
		using Color = glm::vec4;
	public:
		constexpr static auto COMPONENT_ID = 2;

	private:
		Framebuffer paintspace_fbo;
		Texture& paint_mask;

		Shader* painter_shader;
		Texture* stain_tex; // tex to apply to impact
		Color paint_color;
		

	public:
		PaintableComponent(Entity& parent, Shader& painter_shader, Texture& stain_tex, Color paint_color, unsigned int paintmask_width, unsigned int paintmask_height) :
			Component(parent),
			paintspace_fbo    { paintmask_width, paintmask_height, {GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE}, {GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT} },
			paint_mask        { paintspace_fbo.get_color_attachment() },
			painter_shader    { &painter_shader },
			stain_tex         { &stain_tex },
			paint_color       { paint_color }
		{
			paintspace_fbo.bind();
			paint_mask.bind();
			Texture::FormatInfo tx_format = paint_mask.format_info();
			
			// Fill mask with max value
			std::vector<GLubyte> pixels(paintmask_width * paintmask_height, (GLubyte)0xffffffff);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintmask_width, paintmask_height, tx_format.format, tx_format.data_type, pixels.data());
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			GLuint borderColor[] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
			glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			
			// set up parents material to add paintmap as a detail map 
			parent.material->detail_map = &paint_mask;
			
			paint_mask.unbind();
			paintspace_fbo.unbind();
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

		void update_paintmap(const glm::mat4& paintspace_matrix, const glm::vec3& paint_direction)
		{
			paintspace_fbo.bind();
			painter_shader->bind();
			painter_shader->setMat4("paintSpaceMatrix", paintspace_matrix);
			painter_shader->setVec3("paintBallDirection", paint_direction);
			
			// setup stain
			glActiveTexture(GL_TEXTURE0);
			stain_tex->bind();
			painter_shader->setInt("stainTex", 0);
			
			// paint mask
			painter_shader->setInt("paint_map_size", paint_mask.width());
			glBindImageTexture(1, paint_mask.id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8UI);

			
			glClear(GL_DEPTH_BUFFER_BIT);
			parent->custom_draw(*painter_shader);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			paintspace_fbo.unbind();

			ExportTexture(paint_mask.id, paint_mask.width(), paint_mask.width(), "test.bmp", GL_RGB);
		}

		//void on_collision(scene::Entity& other, glm::vec3 contact_point) 
		//{
		//	
		//};
		
		// TODO temp
		void ExportTexture(GLint texture, GLint width, GLint height, std::string name, GLenum format)
		{
			// Image Writing
			unsigned char* imageData;
			if (format == GL_RGB)
				imageData = (unsigned char*)malloc((int)(width * height * 3));
			else if (format == GL_DEPTH_COMPONENT)
				imageData = (unsigned char*)malloc((int)(width * height));
			else
			{
				std::cout << "ExportTexture: unknown format id" << std::endl;
				return;
			}
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, imageData);
			//write 
			bitmap_image image(width, height);
			image_drawer draw(image);

			for (unsigned int i = 0; i < image.width(); ++i)
			{
				for (unsigned int j = 0; j < image.height(); ++j)
				{
					if (format == GL_RGB)
						image.set_pixel(height - j - 1, width - i - 1, *(++imageData), *(++imageData), *(++imageData));
					else
						image.set_pixel(height - j - 1, width - i - 1, *(++imageData), *imageData, *imageData);
				}
			}

			image.save_image(name);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	private:

		
	};
}