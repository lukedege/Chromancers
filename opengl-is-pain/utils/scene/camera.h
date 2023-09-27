#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::scene
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
		glm::vec3 world_front {0, 0, -1}, world_up{0, 1, 0};

		// matrix related
		float fov{ 45.f }, aspect_ratio{ 16.f / 9.f }, near_plane{ 0.1f }, far_plane { 100.f };

		// matrices
		glm::mat4 view_matrix;
		glm::mat4 proj_matrix { glm::perspective(fov, aspect_ratio, near_plane, far_plane) };

	public:
		enum Directions
		{
			FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
		};

		Camera(glm::vec3 pos = {0,0,0}, bool on_ground = true) : 
			pos{pos}, on_ground{on_ground}
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
			view_matrix = glm::lookAt(pos, pos + front, up);
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

		glm::mat4 viewMatrix()
		{
			return view_matrix;
		}

		glm::mat4 projectionMatrix()
		{
			return proj_matrix;
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

		void lookAt(const glm::vec3& position)
		{
			view_matrix = glm::lookAt(pos, position, front);
		}

		void set_position(const glm::vec3& position)
		{
			pos = position;
			view_matrix = glm::lookAt(pos, pos + front, up);
		}

		void set_fov(float new_fov) { fov = new_fov; updateProjectionMatrix(); }
		void set_aspect_ratio(float new_aspect_ratio) { aspect_ratio = new_aspect_ratio; updateProjectionMatrix(); }

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

			view_matrix = glm::lookAt(pos, pos + front, up);
		}

		void updateProjectionMatrix()
		{
			proj_matrix = glm::perspective(fov, aspect_ratio, near_plane, far_plane);
		}
	};
}