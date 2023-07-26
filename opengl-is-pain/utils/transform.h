#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace utils::graphics::opengl
{
	class Transform
	{
		//Local space information
		glm::vec3 _position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 _orientation = { 0.0f, 0.0f, 0.0f }; // euler angles for now
		glm::vec3 _size = { 1.0f, 1.0f, 1.0f };

		// Local -> World matrix
		glm::mat4 _world_matrix{1.0f}; 

		bool dirty = false; // Use when space info is updated to recalculate matrix

	public:
		inline glm::vec3 position   () const noexcept { return _position;    }
		inline glm::vec3 orientation() const noexcept { return _orientation; }
		inline glm::vec3 size       () const noexcept { return _size;        }

		void set_position(const glm::vec3 new_position   ) { dirty = true; _position = new_position; }
		void set_rotation(const glm::vec3 new_orientation) { dirty = true; _orientation = new_orientation; }
		void set_size    (const glm::vec3 new_size       ) { dirty = true; _size = new_size;            }

		void translate(const glm::vec3 translation) { dirty = true; _position += translation; }
		void rotate   (const glm::vec3 rotation   ) { dirty = true; _orientation += rotation; }
		void scale    (const glm::vec3 scale      ) { dirty = true; _size *= scale;           }

		glm::mat4 world_matrix()
		{
			if (dirty)
			{
				glm::mat4 rot_X = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.x), glm::vec3(1.0f, 0.0f, 0.0f));
				glm::mat4 rot_Y = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.y), glm::vec3(0.0f, 1.0f, 0.0f));
				glm::mat4 rot_Z = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.z), glm::vec3(0.0f, 0.0f, 1.0f));

				// Y * X * Z
				glm::mat4 rotation_matrix = rot_Y * rot_X * rot_Z;

				glm::mat4 scale_matrix = glm::scale(glm::mat4{ 1.0f }, _size);
				glm::mat4 translation_matrix = glm::translate(glm::mat4{ 1.0f }, _position);

				_world_matrix = translation_matrix * rotation_matrix * scale_matrix;
				dirty = false;
			}
			
			return _world_matrix;
		}

	};
}