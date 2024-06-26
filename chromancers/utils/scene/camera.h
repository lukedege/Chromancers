#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../utils.h"

namespace engine::scene
{
	// Class for managing a simple first person camera
	class Camera
	{ 
		// input related
		const float YAW = -90.f;
		const float PITCH = 0.f;
		const float ROLL = 0.f;

		const float SPEED = 6.f;
		const float SENSITIVITY = 0.05f;

		float mov_speed{ SPEED }, mouse_sensitivity{ SENSITIVITY };
		bool on_ground;

		// angles
		float yaw{ YAW }, pitch{ PITCH }, roll{ ROLL };

		// vectors
		glm::vec3 _position, front, up, right;
		glm::vec3 world_front {0, 0, 1}, world_up{0, 1, 0};

		// matrix related
		float fov{ 45.f }, aspect_ratio{ 16.f / 9.f }, _near_plane{ .1f }, _far_plane { 100.f };

		// matrices
		glm::mat4 view_matrix;
		glm::mat4 proj_matrix { glm::perspective(fov, aspect_ratio, _near_plane, _far_plane) };

	public:

		Camera(glm::vec3 pos = {0,0,0}, bool on_ground = true) : 
			_position{pos}, on_ground{on_ground}
		{
			updateCameraVectors();
		}

		Camera           (const Camera& copy) = default;
		Camera& operator=(const Camera& copy) = default;

		Camera(Camera&& move) noexcept
			: _position{ move._position }, on_ground{ move.on_ground }, world_up{ move.world_up }
			// TODO add yaw, pitch, roll and other variables to move over
		{
			updateCameraVectors();
		}

		Camera& operator=(Camera&& move) noexcept
		{
			_position = move._position;
			on_ground = move.on_ground;
			world_up = move.world_up;
			updateCameraVectors();
			return *this;
		}

		glm::mat4 viewMatrix()
		{
			return view_matrix;
		}
		glm::mat4 projectionMatrix()
		{
			return proj_matrix;
		}

		utils::math::Frustum frustum()
		{
			utils::math::Frustum frustum;
			const float halfVSide = _far_plane * tanf(fov * .5f);
			const float halfHSide = halfVSide * aspect_ratio;
			const glm::vec3 frontMultFar = _far_plane * front;

			frustum.nearFace = { _position + _near_plane * front, front };
			frustum.farFace  = { _position + frontMultFar, -front };

			frustum.rightFace = { _position,
									glm::cross(frontMultFar - right * halfHSide, up) };
			frustum.leftFace  = { _position,
									glm::cross(up,frontMultFar + right * halfHSide) };

			frustum.topFace    = { _position,
									glm::cross(right, frontMultFar - up * halfVSide) };
			frustum.bottomFace = { _position,
									glm::cross(frontMultFar + up * halfVSide, right) };

			return frustum;
		}

		glm::vec3 position()
		{
			return _position;
		}
		glm::vec3 rotation()
		{
			return { roll, yaw, pitch }; // x, y, z
		}
		glm::vec3 forward()
		{
			return front;
		}

		float near_plane()
		{
			return _near_plane;
		}

		float far_plane()
		{
			return _far_plane;
		}

		void lookAt(const glm::vec3& target, const glm::vec3 up)
		{
			view_matrix = glm::lookAt(_position, target, up);
		}
		void set_position(const glm::vec3& position)
		{
			_position = position;
			lookAt(_position + front, up);
		}
		void set_fov(float new_fov) { fov = new_fov; updateProjectionMatrix(); }
		void set_aspect_ratio(float new_aspect_ratio) { aspect_ratio = new_aspect_ratio; updateProjectionMatrix(); }
		void set_planes(float new_nearplane, float new_farplane) { _near_plane = new_nearplane; _far_plane = new_farplane; updateProjectionMatrix(); }

#pragma region input_related
		enum Directions
		{
			FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
		};

		void ProcessKeyboard(Directions dir, float deltaTime)
		{
			float vel = mov_speed * deltaTime;
			glm::vec3 grounded_front = glm::vec3{ front.x, 0, front.z };

			if (dir == Directions::FORWARD )
			{
				_position += (on_ground ? grounded_front : front) * vel;
			}
			if (dir == Directions::BACKWARD)
			{
				_position -= (on_ground ? grounded_front : front) * vel;
			}
			if (dir == Directions::LEFT )
			{
				_position -= right * vel;
			}
			if (dir == Directions::RIGHT)
			{
				_position += right * vel;
			}
			if (dir == Directions::UP   && !on_ground)
			{
				_position += world_up * vel;
			}
			if (dir == Directions::DOWN && !on_ground)
			{
				_position -= world_up * vel;
			}
			view_matrix = glm::lookAt(_position, _position + front, up);
		}

		void ProcessMouseMovement(float x_offset, float y_offset, bool pitch_constraint = true)
		{
			x_offset *= mouse_sensitivity; y_offset *= mouse_sensitivity;

			yaw += x_offset; pitch += y_offset;

			if (pitch_constraint) // avoids gimbal lock
			{
				pitch = glm::clamp(pitch, -89.f, 89.f);
			}

			updateCameraVectors();
		}

		void toggle_fly()
		{
			on_ground = !on_ground;
		}
#pragma endregion input_related

	private:
		void updateCameraVectors()
		{
			float yaw_r   = glm::radians(yaw  ),
				  pitch_r = glm::radians(pitch),
				  roll_r  = glm::radians(roll );

			front.x = cos(yaw_r) * cos(pitch_r);
			front.y = sin(pitch_r);
			front.z = sin(yaw_r) * cos(pitch_r);

			//world_front = front = glm::normalize(front);
			//world_front.y = 0;

			right = glm::normalize(glm::cross(front, world_up));
			up    = glm::normalize(glm::cross(right, front));

			lookAt(_position + front, up);
		}

		void updateProjectionMatrix()
		{
			proj_matrix = glm::perspective(fov, aspect_ratio, _near_plane, _far_plane);
		}
	};
}