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
#include "material.h"

// Model class purpose:
// 1. Open file from disk
// 2. Load data with assimp
// 3. Pass all nodes to data structure
// 4. Create a mesh from data structure (which will setup VBO)

namespace engine::resources
{
	class Model
	{
		struct TextureEntry
		{
			Texture tex;
			std::string type;
			std::string path;  // we store the path of the texture to compare with other textures
		};
		struct MaterialEntry
		{
			Material material;
			std::string name;
			std::vector<TextureEntry*> associated_textures;
		};
		struct MeshEntry
		{
			Mesh mesh;
			unsigned int associated_material_idx; // or MaterialEntry associated_material
		};
		

	public:
		std::vector<MeshEntry> meshes;
		std::vector<MaterialEntry> materials;
		std::vector<TextureEntry> textures_loaded;

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

		Model(const Model& copy) = delete;
		Model& operator=(const Model& copy) noexcept = delete;

		Model(Model&& move) = default;
		Model& operator=(Model&& move) noexcept = default;

		void draw() const
		{
			for (size_t i = 0; i < meshes.size(); i++)
			{
				meshes[i].mesh.draw();
			}
		}

		void draw(Shader& shader)
		{
			for (size_t i = 0; i < meshes.size(); i++)
			{
				if (has_material())
				{
					auto& current_mats = materials[meshes[i].associated_material_idx];
					current_mats.material.shader = &shader;
					current_mats.material.bind();
					meshes[i].mesh.draw();
					current_mats.material.unbind();
				}
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

		bool has_material() // different from auto-imported default material
		{
			return materials.size() > 1;
		}

	private:
		Model(MeshEntry& mesh) : meshes{}
		{
			meshes.push_back(std::move(mesh));
		}

		Model(MeshEntry&& mesh) : meshes{}
		{
			meshes.push_back(std::move(mesh));
		}

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
			textures_loaded.reserve(32); // TODO this is a temporary patchfix, better indexing needed instead of using textureentry* for the materials

			// First material is default
			for (unsigned int i = 0 ; i < scene->mNumMaterials ; i++) 
			{
				aiMaterial* pMaterial = scene->mMaterials[i];
				aiString name = pMaterial->GetName();

				MaterialEntry new_mat;
				new_mat.name = name.C_Str();
				new_mat.associated_textures = {};
				std::vector<TextureEntry*> diffuse_tex = loadMaterialTextures(pMaterial, aiTextureType_DIFFUSE, "diffuse_tex", Dir);
				new_mat.associated_textures.insert(new_mat.associated_textures.end(), diffuse_tex.begin(), diffuse_tex.end());
				if (diffuse_tex.size() > 0) new_mat.material.diffuse_map = &diffuse_tex[0]->tex;

				std::vector<TextureEntry*> normals_tex = loadMaterialTextures(pMaterial, aiTextureType_NORMALS, "normals_tex", Dir);
				new_mat.associated_textures.insert(new_mat.associated_textures.end(), normals_tex.begin(), normals_tex.end());
				if (normals_tex.size() > 0) new_mat.material.normal_map = &normals_tex[0]->tex;

				
				new_mat.material.uv_repeat = 1.f;

				// Fill out other material properties

				materials[i] = new_mat;
			}


		}

		std::vector<TextureEntry*> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, std::string base_path)
		{
			std::vector<TextureEntry*> textures;
			for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString tex_path;
				mat->GetTexture(type, i, &tex_path);
				std::string full_path = base_path + "/" + tex_path.data;

				bool skip = false;
				for(unsigned int j = 0; j < textures_loaded.size(); j++)
				{
					if(std::strcmp(textures_loaded[j].path.data(), full_path.c_str()) == 0)
					{
						// save ref to texture (idx/ptr?)
						textures.push_back(&textures_loaded[j]);
						skip = true; 
						break;
					}
				}
				if(!skip)
				{   // if texture hasn't been loaded already, load it
					TextureEntry* t = &textures_loaded.emplace_back(Texture{ full_path, false }, typeName, full_path); // add to loaded textures

					textures.push_back(t);
				}
			}
			return textures;
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