#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilites.h"

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice new_physical_device, VkDevice newDevice, std::vector<Vertex>* vertices);

    int GetVertexCount();
    VkBuffer GetVertexBuffer();

    void DestroyVertexBuffer();

    ~Mesh();

private:
    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkBuffer CreateVertexBuffer(std::vector<Vertex>* vertices);
    uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties);
};