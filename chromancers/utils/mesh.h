#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <gsl/gsl>

#include <glad.h>
#include <glm/glm.hpp>

namespace engine::resources
{
	//unifica con model, funzione statica per caricare obj con più sottomesh
	struct Vertex
	{
		glm::vec3 position; 
		glm::vec2 texCoords;
		glm::vec3 normal, tangent, bitangent;
	};

	class Mesh
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		GLuint VAO;

		Mesh(std::vector<Vertex>& v, std::vector<GLuint>& i) noexcept :
			vertices(std::move(v)), indices(std::move(i)) {
			setupMesh();
		}

		Mesh(std::vector<Vertex>&& v, std::vector<GLuint>&& i) noexcept :
			vertices(std::move(v)), indices(std::move(i)) {
			setupMesh();
		}

		           Mesh(const Mesh& copy) = delete;
		Mesh& operator=(const Mesh& copy) = delete;

		Mesh(Mesh&& move) noexcept :
			vertices(std::move(move.vertices)), indices(std::move(move.indices)),
			VAO(move.VAO), VBO(move.VBO), EBO(move.EBO)
		{
			move.VAO = 0;
		}

		Mesh& operator=(Mesh&& move) noexcept
		{
			dispose();

			// Check if it exists
			if (move.VAO)
			{
				vertices = std::move(move.vertices);
				indices = std::move(move.indices);
				VAO = move.VAO; VBO = move.VBO; EBO = move.EBO;

				move.VAO = 0;
			}
			else
			{
				VAO = 0;
			}

			return *this;
		}

		~Mesh()
		{
			dispose();
		}

		void draw(GLenum mode = GL_TRIANGLES) const
		{
			glBindVertexArray(VAO);
			glDrawElements(mode, gsl::narrow<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		void draw_instanced(size_t amount, GLenum mode = GL_TRIANGLES) const
		{
			glBindVertexArray(VAO);
			glDrawElementsInstanced(mode, gsl::narrow<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0, gsl::narrow<GLsizei>(amount));
			glBindVertexArray(0);
		}

		std::vector<glm::vec3> get_vertices_positions() const
		{
			std::vector<glm::vec3> ret_vertices;

			for (const Vertex& v : vertices)
				ret_vertices.push_back(v.position);
			return ret_vertices;
		}

		// Creates and returns a simple triangle mesh
		static Mesh simple_triangle_mesh()
		{
			return Mesh
			{
				std::vector<Vertex>
				{
					Vertex{ glm::vec3{-0.5f, -0.5f, 0.0f} /* position */ },
					Vertex{ glm::vec3{ 0.5f, -0.5f, 0.0f} /* position */ },
					Vertex{ glm::vec3{ 0.0f,  0.5f, 0.0f} /* position */ },
				},
					std::vector<GLuint>{0, 1, 2}
			};
		}

		// Creates and returns a simple triangle mesh
		static Mesh simple_quad_mesh()
		{
			float size = 1.f;
			return Mesh
			{
				std::vector<Vertex>
				{
					Vertex{ glm::vec3{-size, -size, 0.0f} /* position */, glm::vec2{ 0.0f, 0.0f } /* texcoords*/ },
					Vertex{ glm::vec3{ size, -size, 0.0f} /* position */, glm::vec2{ 1.0f, 0.0f } /* texcoords*/ },
					Vertex{ glm::vec3{ size,  size, 0.0f} /* position */, glm::vec2{ 1.0f, 1.0f } /* texcoords*/ },
					Vertex{ glm::vec3{-size,  size, 0.0f} /* position */, glm::vec2{ 0.0f, 1.0f } /* texcoords*/ },
				},
				std::vector<GLuint>
				{
					0, 1, 2, // first  triangle
					0, 2, 3  // second triangle
				}
			};
		}

	private:
		GLuint VBO, EBO;

		void setupMesh()
		{
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			// VAO is made "active"    
			glBindVertexArray(VAO);
			// we copy data in the VBO - we must set the data dimension, and the pointer to the structure cointaining the data
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
			// we copy data in the EBO - we must set the data dimension, and the pointer to the structure cointaining the data
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			// positions (location = 0 in shader)
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);

			// normals (location = 1 in shader)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));

			// texcoords (location = 2 in shader)
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoords));

			// tangent (location = 3 in shader)
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));

			// bitangent (location = 4 in shader)
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));

			glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
			glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

		}

		void dispose()
		{
			// Check if we have something in GPU
			if (VAO)
			{
				glDeleteVertexArrays(1, &VAO);
				glDeleteBuffers(1, &VBO);
				glDeleteBuffers(1, &EBO);
			}
		}
	};
}