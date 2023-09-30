#pragma once

#include <CodeAnalysis/Warnings.h>
#pragma warning ( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

#pragma warning ( pop )

namespace engine
{
	class Transform
	{
		//Local space information
		glm::vec3 _position    { 0.0f, 0.0f, 0.0f };
		glm::vec3 _orientation { 0.0f, 0.0f, 0.0f }; // euler angles for now
		glm::vec3 _size        { 1.0f, 1.0f, 1.0f };

		// Local -> World matrix
		glm::mat4 _world_matrix { 1.0f }; // TODO TEMPORARILY PUBLIC 

		bool dirty = false; // Use when space info is updated to recalculate matrix

	public:
		inline glm::vec3 position   () const noexcept { return _position;    }
		inline glm::vec3 orientation() const noexcept { return _orientation; }
		inline glm::vec3 size       () const noexcept { return _size;        }

		void set(const glm::mat4& matrix) noexcept
		{
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(matrix, _size, rotation, _position, skew, perspective);
			_orientation = glm::eulerAngles(rotation);

			_world_matrix = matrix;
		}

		void set_position(const glm::vec3& new_position   ) { _position = new_position;       update_world_matrix(); }
		void set_rotation(const glm::vec3& new_orientation) { _orientation = new_orientation; update_world_matrix(); }
		void set_size    (const glm::vec3& new_size       ) { _size = new_size;               update_world_matrix(); }

		void translate(const glm::vec3& translation) { _position += translation; update_world_matrix(); }
		void rotate   (const glm::vec3& rotation   ) { _orientation += rotation; update_world_matrix(); }
		void scale    (const glm::vec3& scale      ) { _size *= scale;           update_world_matrix(); }

		const glm::mat4& world_matrix() const noexcept
		{	
			return _world_matrix;
		}

	private:
		void update_world_matrix()
		{
			// TODO start using quats
			//glm::quat rotation_quat {_orientation};
			//glm::mat4 rotation_matrix = glm::toMat4(rotation_quat);
			
			// Y * X * Z
			glm::mat4 rot_X = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rot_Y = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rot_Z = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotation_matrix = rot_Y * rot_X * rot_Z;

			glm::mat4 scale_matrix = glm::scale(glm::mat4{ 1.0f }, _size);
			glm::mat4 translation_matrix = glm::translate(glm::mat4{ 1.0f }, _position);

			_world_matrix = translation_matrix * rotation_matrix * scale_matrix;
		}
	};
}