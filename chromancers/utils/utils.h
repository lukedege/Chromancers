#pragma once
#include <string>

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// Various and miscellaneous utilities are located here
namespace utils::strings
{
	// Erases the whole line at the specified string index (if valid)
	// TODO: when the index points at a "\n" character the line isnt deleted, to fix or give meaning to
	//       For now, this isn't a problem since it's only used in junction with find functions
	inline std::string eraseLineAt(const std::string source, const size_t pos)
	{
		std::string ret = source;
		if (pos < source.length())
		{
			size_t posNextNL = source.find("\n", pos);
			size_t posPrevNL = source.rfind("\n", pos);

			if (posPrevNL == std::string::npos) posPrevNL = 0;
			if (posNextNL == std::string::npos) posNextNL = source.length();

			ret.erase(posPrevNL, posNextNL - posPrevNL);
		}

		return ret;
	}

	// Erases first line found containing toErase string from source string
	inline std::string eraseLineContaining(const std::string source, const std::string toErase)
	{
		size_t posContent = source.find(toErase);
		std::string ret = eraseLineAt(source, posContent);

		return ret;
	}

	// Erases all lines containing toErase string
	inline std::string eraseAllLinesContaining(const std::string source, const std::string toErase)
	{
		std::string ret = source;
		size_t posContent = ret.find(toErase);
		while (posContent != std::string::npos)
		{
			ret = eraseLineAt(ret, posContent);
			posContent = ret.find(toErase);
		}

		return ret;
	}
}

namespace utils::math
{
	// Simple mathematical plane
	struct Plane
	{
		glm::vec3 normal   { 0.f, 1.f, 0.f }; // Unit vector
		float     distance { 0.f };           // Distance from origin

		Plane() = default;

		Plane(const glm::vec3& p1, const glm::vec3& norm) : 
			normal  (glm::normalize(norm)),
			distance(glm::dot(normal, p1))
		{}

		float getSignedDistanceToPlane(const glm::vec3& point) const
		{
			return glm::dot(normal, point) - distance;
		}
	};
   
	// Simple frustum volume representation
	struct Frustum
	{
		Plane topFace;
		Plane bottomFace;

		Plane rightFace;
		Plane leftFace;

		Plane farFace;
		Plane nearFace;
	};

	inline glm::vec4 unproject(float screen_x, float screen_y, int screen_width, int screen_height, glm::mat4 view, glm::mat4 proj)
	{
		glm::vec4 ret{1};
		glm::mat4 unproject_mat;

		// we must retro-project the coordinates of the mouse pointer, in order to have a point in world coordinate to be used to determine a vector from the camera (= direction and orientation of the bullet)
		// we convert the cursor position (taken from the mouse callback) from Viewport Coordinates to Normalized Device Coordinate (= [-1,1] in both coordinates)
		ret.x = (screen_x / screen_width) * 2.0f - 1.0f;
		ret.y = -(screen_y / screen_height) * 2.0f + 1.0f; // Viewport Y coordinates are from top-left corner to the bottom
		// we need a 3D point, so we set a minimum value to the depth with respect to camera position
		ret.z = 1.0f;
		// w = 1.0 because we are using homogeneous coordinates
		ret.w = 1.0f;

		// we determine the inverse matrix for the projection and view transformations
		unproject_mat = glm::inverse(proj * view);

		// we convert the position of the cursor from NDC to world coordinates, and we multiply the vector by the initial speed
		ret = glm::normalize(unproject_mat * ret);

		return ret;
	}
}

namespace utils::graphics::opengl
{
	// Setups an OpenGL buffer object and fills it with the provided data
	inline void setup_buffer_object(GLuint& buffer_object, GLenum target, int bind_index, size_t alloc_size, GLenum usage, void* data = nullptr)
	{
		glBindBuffer(target, buffer_object);

		glBufferData(target, alloc_size, NULL, usage); // allocate alloc_size bytes of memory
		glBindBufferBase(target, bind_index, buffer_object);

		if (data != 0)
			glBufferSubData(target, 0, alloc_size, data);        // fill buffer object with data

		glBindBuffer(target, 0);
	}

	// Setups an OpenGL buffer object and fills it with the provided data (overload that automatically calculates allocation size given size and amount)
	inline void setup_buffer_object(GLuint& buffer_object, GLenum target, int bind_index, size_t element_size, size_t element_amount, GLenum usage, void* data = nullptr)
	{
		size_t alloc_size = element_size * element_amount;
		setup_buffer_object(buffer_object, target, bind_index, alloc_size, usage, data);
	}	

	// Updates the content of an OpenGL buffer object at the required offset with the provided data
	inline void update_buffer_object(GLuint& buffer_object, GLenum target, size_t offset, size_t element_size, size_t element_amount, void* data = nullptr)
	{
		size_t alloc_size = element_size * element_amount;
		glBindBuffer(target, buffer_object);
		glBufferSubData(target, offset, alloc_size, data);
		glBindBuffer(target, 0);
	}
}

namespace utils::containers
{
	// Custom limited-size queue implementation 
	// T      : type of contained object
	// MaxLen : maximum size 
	template <typename T, int MaxLen>
	class FixedQueue : public std::vector<T> {
	public:
		FixedQueue() :
			std::vector<T>()
		{
			this->resize(MaxLen);
		}

		// When adding a new value, delete the oldest one (in our case, the one in the front)
		void push_back(const T& value) 
		{
			if (this->size() == MaxLen) {
				this->erase(this->begin());
			}
			std::vector<T>::push_back(value);
		}
	};
}