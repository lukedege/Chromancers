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
		glm::vec3 _position    { 0.0f, 0.0f, 0.0f };
		glm::vec3 _orientation { 0.0f, 0.0f, 0.0f }; // euler angles for now
		glm::vec3 _size        { 1.0f, 1.0f, 1.0f };

		// Resultant matrix
		glm::mat4 _matrix { 1.0f };

		// World axes, inits equal as local axes
		glm::vec3 _forward { 0.0f, 0.0f, 1.0f };
		glm::vec3 _right   { 1.0f, 0.0f, 0.0f };
		glm::vec3 _up      { 0.0f, 1.0f, 0.0f };

		bool dirty = false; // Use when space info is updated to recalculate matrix

	public:
		Transform() 
		{
			update_matrix();
		}

		inline glm::vec3 position   () const noexcept { return _position;    }
		inline glm::vec3 orientation() const noexcept { return _orientation; }
		inline glm::vec3 size       () const noexcept { return _size;        }

		inline glm::vec3 forward  () const noexcept { return _forward; }
		inline glm::vec3 right    () const noexcept { return _right; }
		inline glm::vec3 up       () const noexcept { return _up; }

		void set(const glm::mat4& matrix) noexcept
		{
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(matrix, _size, rotation, _position, skew, perspective);
			_orientation = glm::degrees(glm::eulerAngles(rotation));

			glm::mat3 rot_3{rotation};
			_forward = glm::normalize(rot_3 * _forward);
			_right   = glm::normalize(rot_3 * _right  );
			_up      = glm::normalize(rot_3 * _up     );

			_matrix = matrix;
		}

		void set_position   (const glm::vec3& new_position   ) { _position = new_position;       update_matrix(); }
		void set_orientation(const glm::vec3& new_orientation) { _orientation = new_orientation; update_matrix(); }
		void set_size       (const glm::vec3& new_size       ) { _size = new_size;               update_matrix(); }

		void translate(const glm::vec3& translation) { _position += translation; update_matrix(); }
		void rotate   (const glm::vec3& rotation   ) { _orientation += rotation; update_matrix(); }
		void scale    (const glm::vec3& scale      ) { _size *= scale;           update_matrix(); }

		const glm::mat4& matrix() const noexcept
		{	
			return _matrix;
		}

		friend Transform operator*(const Transform& lhs, const Transform& rhs)
		{
			Transform result;
			glm::mat4 m_result = lhs.matrix() * rhs.matrix();
			result.set(m_result);
			return result;
		}

	private:
		void update_matrix()
		{
			// TODO start using quats
			//glm::quat rotation_quat {_orientation};
			//glm::mat4 rotation_matrix = glm::toMat4(rotation_quat);
			
			// Y * X * Z
			glm::mat4 rot_X = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rot_Y = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rot_Z = glm::rotate(glm::mat4{ 1.0f }, glm::radians(_orientation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotation_matrix = rot_Y * rot_X * rot_Z;

			glm::mat3 rot_3{rotation_matrix};
			_forward = glm::normalize(glm::vec3{ 0.0f, 0.0f, 1.0f } * rot_3);
			_right   = glm::normalize(glm::vec3{ 1.0f, 0.0f, 0.0f } * rot_3);
			_up      = glm::normalize(glm::vec3{ 0.0f, 1.0f, 0.0f } * rot_3);

			glm::mat4 scale_matrix = glm::scale(glm::mat4{ 1.0f }, _size);
			glm::mat4 translation_matrix = glm::translate(glm::mat4{ 1.0f }, _position);

			_matrix = translation_matrix * rotation_matrix * scale_matrix;
		}
	};

	
}