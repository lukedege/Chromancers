#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace utils::graphics::opengl
{
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
		glm::vec3 pos, front, up, right;
		glm::vec3 world_front, world_up;

		// matrix related
		float fov, aspect_ratio, near_plane, far_plane;

	public:
		enum Directions
		{
			FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
		};

		Camera(glm::vec3 pos = {0,0,0}, bool on_ground = true) : pos{pos}, on_ground{on_ground},
			world_up{ 0, 1, 0 }
		{
			updateCameraVectors();
		}

		Camera(const Camera& copy) = delete;
		Camera& operator=(const Camera& copy) = delete;

		Camera(Camera&& move) noexcept
			: pos{ move.pos }, on_ground{ move.on_ground }, world_up{ move.world_up }
			// TODO add yaw, pitch, roll and other variables to move over
		{
			updateCameraVectors();
		}

		Camera& operator=(Camera&& move) noexcept
		{
			pos = move.pos;
			on_ground = move.on_ground;
			world_up = move.world_up;
			updateCameraVectors();
			return *this;
		}

		void ProcessKeyboard(Directions dir, float deltaTime)
		{
			float vel = mov_speed * deltaTime;

			if (dir == Directions::FORWARD )
			{
				pos += (on_ground ? world_front : front) * vel;
			}
			if (dir == Directions::BACKWARD)
			{
				pos -= (on_ground ? world_front : front) * vel;
			}
			if (dir == Directions::LEFT )
			{
				pos -= right * vel;
			}
			if (dir == Directions::RIGHT)
			{
				pos += right * vel;
			}
			if (dir == Directions::UP   && !on_ground)
			{
				pos += world_up * vel;
			}
			if (dir == Directions::DOWN && !on_ground)
			{
				pos -= world_up * vel;
			}
		}

		void ProcessMouseMovement(float x_offset, float y_offset, bool pitch_constraint = true)
		{
			x_offset *= mouse_sensitivity; y_offset *= mouse_sensitivity;

			yaw += x_offset; pitch += y_offset;

			if (pitch_constraint) // avoids gimbal lock
			{
				glm::clamp(pitch, -89.f, 89.f);
			}

			updateCameraVectors();
		}

		void toggle_fly()
		{
			on_ground = !on_ground;
		}

		glm::mat4 view_matrix()
		{
			return glm::lookAt(pos, pos + front, up);
		}

		glm::mat4 projection_matrix()
		{
			return glm::perspective(45.0f, 16.f / 9.f, 0.1f, 100.0f);
		}

		glm::vec3 position()
		{
			return pos;
		}

		glm::vec3 rotation()
		{
			return { roll, yaw, pitch };
		}

		glm::vec3 forward()
		{
			return front;
		}

	private:
		void updateCameraVectors()
		{
			// TODO add roll
			float yaw_r   = glm::radians(yaw  ),
				  pitch_r = glm::radians(pitch),
				  roll_r  = glm::radians(roll );

			front.x = cos(yaw_r) * cos(pitch_r);
			front.y = sin(pitch_r);
			front.z = sin(yaw_r) * cos(pitch_r);

			world_front = front = glm::normalize(front);
			world_front.y = 0;

			right = glm::normalize(glm::cross(front, world_up));
			up    = glm::normalize(glm::cross(right, front));
		}
	};
}