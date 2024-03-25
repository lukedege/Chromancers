#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <optional>

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
#include "texture.h"

// Model class purpose:
// 1. Open file from disk
// 2. Load data with assimp
// 3. Pass all nodes to data structure
// 4. Create a mesh from data structure (which will setup VBO)

namespace engine::resources
{
	class Model
	{
		struct MeshEntry
		{
			Mesh mesh;
			unsigned int material_idx;
		};

	public:
		std::vector<MeshEntry> meshes;
		std::vector<std::optional<Texture>> materials;

		// Create model from file
		Model(const std::string& path) { loadModel(path); }

		// Create model with an inital existing mesh
		Model(Mesh& mesh) : meshes{}
		{
			meshes.push_back({ std::move(mesh), 0 });
		}

		Model(Mesh&& mesh) : meshes{}
		{
			meshes.push_back({ std::move(mesh), 0 });
		}
		
		Model(MeshEntry& mesh) : meshes{}
		{
			meshes.push_back(std::move(mesh));
		}

		Model(MeshEntry&& mesh) : meshes{}
		{
			meshes.push_back(std::move(mesh));
		}

		Model(const Model& copy) = delete;
		Model& operator=(const Model& copy) noexcept = delete;

		Model(Model&& move) = default;
		Model& operator=(Model&& move) noexcept = default;

		void draw(bool use_own_material = false) const
		{
			for (size_t i = 0; i < meshes.size(); i++)
			{
				if (use_own_material && materials[meshes[i].material_idx].has_value())
				{
					glActiveTexture(GL_TEXTURE0);
					materials[meshes[i].material_idx].value().bind();
				}
				meshes[i].mesh.draw();
			}
		}

		void draw_instanced(size_t amount) const
		{
			for (size_t i = 0; i < meshes.size(); i++) { meshes[i].mesh.draw_instanced(amount); }
		}

		std::vector<glm::vec3> get_vertices_positions() const
		{
			std::vector<glm::vec3> vertices;

			for (const MeshEntry& m : meshes)
				for (const Vertex& v : m.mesh.vertices)
					vertices.push_back(v.position);
			return vertices;
		}

		bool has_diffuse()
		{
			// TODO temp, 
			return materials.size() > 2;
		}

	private:
		void loadModel(const std::string& path)
		{
			Assimp::Importer importer;

			// Applying various mesh processing functions to the import by assimp
			const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs |
				aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				// If scene failed to complete or there are some error flags from assimp, then stop application
				using namespace std::string_literals;
				throw std::runtime_error{ "ASSIMP ERROR! "s + importer.GetErrorString() + "\n" };
			}

			loadMaterials(scene, path);

			// process the scene tree starting from root node down to its descendants
			meshes = processNode(scene->mRootNode, scene);
		}

		std::vector<MeshEntry> processNode(const aiNode* node, const aiScene* scene)
		{
			std::vector<MeshEntry> meshes;

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
				std::vector<MeshEntry> childMeshes{ processNode(node->mChildren[i], scene) };
				for (size_t i = 0; i < childMeshes.size(); i++)
				{
					meshes.emplace_back(std::move(childMeshes[i]));
				}
			}
			return meshes;
		}

		void loadMaterials(const aiScene* scene, const std::string& path) 
		{
			// Extract the directory part from the file name
			std::string::size_type SlashIndex = path.find_last_of("/");
			std::string Dir;

			if (SlashIndex == std::string::npos) { Dir = "."; }
			else if (SlashIndex == 0) { Dir = "/"; }
			else { Dir = path.substr(0, SlashIndex); }

			// Initialize the materials
			materials.resize(scene->mNumMaterials);
			for (unsigned int i = 0 ; i < scene->mNumMaterials ; i++) {
				const aiMaterial* pMaterial = scene->mMaterials[i];

				if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
					aiString Path;

					if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
						std::string FullPath = Dir + "/" + Path.data;

						auto it = materials.emplace(materials.begin() + i, Texture{ FullPath, false });
					}
				}
			}


		}

		MeshEntry processMesh(const aiMesh* mesh)
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
			
			return { Mesh{ vertices, indices }, mesh->mMaterialIndex };
		}

	};
}