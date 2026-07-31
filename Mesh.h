#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilites.h"

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice new_physical_device, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,  std::vector<Vertex>* vertices, std::vector<uint32_t>* indices);

    int GetVertexCount();
    VkBuffer GetVertexBuffer();

    int GetIndexCount();
    VkBuffer GetIndexBuffer();

    void DestroyBuffers();

    ~Mesh();

private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    int indexCount;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
    void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};