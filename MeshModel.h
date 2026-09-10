#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <assimp/scene.h>

#include "Mesh.h"

class MeshModel
{
public:
     MeshModel();
     MeshModel(std::vector<Mesh> newMeshList);

     size_t GetMeshCount();
     Mesh* GetMesh(size_t index);

     glm::mat4 GetModel();
     void SetModel(glm::mat4 newModel);

     void DestroyMeshModel();

     static std::vector<std::string> LoadMaterials(const aiScene* scene);
     static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex);
     static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex);

     ~MeshModel();

private:
     std::vector<Mesh> meshList;
     glm::mat4 model;
};
