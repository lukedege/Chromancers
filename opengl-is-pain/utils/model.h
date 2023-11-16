#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <CodeAnalysis/Warnings.h>
#pragma warning ( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad.h>
#include <glm/glm.hpp>

#pragma warning ( pop )

#include "mesh.h"

// Model class purpose:
// 1. Open file from disk
// 2. Load data with assimp
// 3. Pass all nodes to data structure
// 4. Create a mesh from data structure (which will setup VBO)

namespace engine::resources
{
	class Model
	{
	public:
		std::vector<Mesh> meshes;

		// Create model from file
		Model(const std::string& path) : meshes{ loadModel(path) }
		{}

		// Create model with an inital existing mesh
		Model(Mesh& mesh) : meshes {}
		{
			meshes.push_back(std::move(mesh));
		}

		Model(Mesh&& mesh) : meshes {}
		{
			meshes.push_back(std::move(mesh));
		}

		Model(const Model& copy) = delete;
		Model& operator=(const Model& copy) noexcept = delete;
		
		Model(Model&& move) = default;
		Model& operator=(Model&& move) noexcept = default;

		void draw() const
		{
			for (size_t i = 0; i < meshes.size(); i++) { meshes[i].draw(); }
		}

		void draw_instanced(size_t amount) const
		{
			for (size_t i = 0; i < meshes.size(); i++) { meshes[i].draw_instanced(amount); }
		}

		std::vector<glm::vec3> get_vertices_positions() const 
		{
			std::vector<glm::vec3> vertices;

			for (const Mesh& m : meshes)
				for (const Vertex& v : m.vertices)
					vertices.push_back(v.position);
			return vertices;
		}

	private:
		std::vector<Mesh> loadModel(const std::string& path)
		{
			Assimp::Importer importer;

			// Applying various mesh processing functions to the import by assimp
			const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs |
				aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace );

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				// If scene failed to complete or there are some error flags from assimp, then stop application
				using namespace std::string_literals;
				throw std::runtime_error{ "ASSIMP ERROR! "s + importer.GetErrorString() + "\n" };
			}

			// process the scene tree starting from root node down to its descendants
			return processNode(scene->mRootNode, scene);
		}

		std::vector<Mesh> processNode(const aiNode* node, const aiScene* scene)
		{
			std::vector<Mesh> meshes;

			// Process each node
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				// each node has an index reference to its mesh, which is contained in scene's mMeshes array
				const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				//auto pippo{ processMesh(mesh) };
				meshes.emplace_back(std::move(processMesh(mesh)));
			}

			// recurse through the nodes children 
			for (size_t i = 0; i < node->mNumChildren; i++)
			{
				std::vector<Mesh> childMeshes{ processNode(node->mChildren[i], scene) };
				for (size_t i = 0; i < childMeshes.size(); i++)
				{
					meshes.emplace_back(std::move(childMeshes[i]));
				}
			}
			return meshes;
		}

		Mesh processMesh(const aiMesh* mesh)
		{
			std::vector<Vertex> vertices;
			std::vector<GLuint>  indices;

			for (size_t i = 0; i < mesh->mNumVertices; i++)
			{
				Vertex vertex;
				glm::vec3 vec3;
				vec3.x = mesh->mVertices[i].x;
				vec3.y = mesh->mVertices[i].y;
				vec3.z = mesh->mVertices[i].z;
				vertex.position = vec3;

				vec3.x = mesh->mNormals[i].x;
				vec3.y = mesh->mNormals[i].y;
				vec3.z = mesh->mNormals[i].z;
				vertex.normal = vec3;

				if (mesh->mTextureCoords[0])
				{
					// The model has UV textures so we assign them
					glm::vec2 vec2;
					vec2.x = mesh->mTextureCoords[0][i].x;
					vec2.y = mesh->mTextureCoords[0][i].y;
					vertex.texCoords = vec2;

					// Since there are UVs, then assimp calculated the tan + bitan
					vec3.x = mesh->mTangents[i].x;
					vec3.y = mesh->mTangents[i].y;
					vec3.z = mesh->mTangents[i].z;
					vertex.tangent = vec3;

					vec3.x = mesh->mBitangents[i].x;
					vec3.y = mesh->mBitangents[i].y;
					vec3.z = mesh->mBitangents[i].z;
					vertex.bitangent = vec3;
				}
				else
				{
					// The model has no UV textures
					vertex.texCoords = glm::vec2(0.f, 0.f);
					std::cout << "Warning: UV not present" << std::endl;
				}

				vertices.emplace_back(vertex);
			}

			for (size_t i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];

				for (size_t j = 0; j < face.mNumIndices; j++)
				{
					indices.emplace_back(face.mIndices[j]);
				}
			}

			Mesh m{ vertices, indices };
			return m;
		}

	};
}