#pragma once

#include "../component.h"
#include "../transform.h"
#include "../shader.h"
#include "../framebuffer.h"

namespace engine::components
{
	// Component that makes an entity paintable by storing a paintmap and making it react to paintball impacts
	class PaintableComponent : public Component
	{
		using Shader = engine::resources::Shader;
		using Texture = engine::resources::Texture;
		using Framebuffer = utils::graphics::opengl::Framebuffer;

	public:
		constexpr static auto COMPONENT_ID = 2;

	private:
		Texture paint_map;         // The paintmap itself
		Texture* paint_normal_map; // The normal map for painted zones of the object
		Texture* splat_tex;        // The texture to apply on paintball impact

		Shader* painter_shader; // The shader which will apply the paintball impacts (splats) onto the paintmap

		Framebuffer paint_fbo; // We use an ad-hoc framebuffer simply to avoid polluting the main rendering framebuffer with drawcalls

	public:

		PaintableComponent(Entity& parent, Shader& painter_shader, unsigned int paintmap_width, unsigned int paintmap_height, Texture* splat_tex, Texture* paint_normal_map = nullptr) :
			Component(parent),
			paint_map { paintmap_width, paintmap_height, {GL_RGBA8, GL_RGBA, GL_FLOAT} },
			painter_shader{ &painter_shader },
			splat_tex{ splat_tex },
			paint_normal_map{ paint_normal_map } 
		{
			// We initialize the paintmap with zeros
			std::vector<GLfloat> pixels(paintmap_width * paintmap_height * 4, 0.f);
			
			const auto& pmap_format = paint_map.format_info();

			paint_map.bind();
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paintmap_width, paintmap_height, pmap_format.format, pmap_format.data_type, pixels.data());

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			paint_map.unbind();

			// We add to the parent entity's material a detail diffusemap and a detail normalmap
			// these will draw on top of the already existing diffuse and normal maps
			parent.material->detail_diffuse_map = &paint_map;
			parent.material->detail_normal_map = paint_normal_map;
		}

		int type()
		{
			return COMPONENT_ID;
		}

		void init() {}

		void update(float delta_time) {}

		void update_paintmap(const glm::mat4& paintspace_matrix, const glm::vec3& paint_direction, const glm::vec4& paint_color)
		{
			paint_fbo.bind();
			{
				painter_shader->bind();
				{
					glClear(GL_DEPTH_BUFFER_BIT);
					// Setup uniforms
					painter_shader->setMat4("paintSpaceMatrix", paintspace_matrix);
					painter_shader->setVec3("paintBallDirection", paint_direction);
					painter_shader->setVec4("paintBallColor", paint_color);

					// Setup splat mask
					glActiveTexture(GL_TEXTURE0);
					splat_tex->bind();
					painter_shader->setInt("splat_mask", 0);

					// Bind paintmap and relative informations
					glBindImageTexture(1, paint_map.id(), 0, GL_FALSE, 0, GL_READ_WRITE, paint_map.format_info().internal_format);
					painter_shader->setInt("paintmap_size", paint_map.width());

					_parent->custom_draw(*painter_shader);
					glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
					// This barrier is needed to ensure that the imageStore operation in the shader 
					// (which saves the modified paintmap) is executed completely before
					// proceeding with the next host instructions
				}
				painter_shader->unbind();
			}
			paint_fbo.unbind();
		}

	};
}