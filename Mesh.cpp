#include "Mesh.h"

#include <cstring>

Mesh::Mesh() {}

Mesh::Mesh(VkPhysicalDevice new_physical_device, VkDevice newDevice, std::vector<Vertex> *vertices)
: vertexCount(vertices->size()), physicalDevice(new_physical_device), device(newDevice)
{
    CreateVertexBuffer(vertices);
}

int Mesh::GetVertexCount()
{
    return vertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
    return vertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{

}

void Mesh::CreateVertexBuffer(std::vector<Vertex> *vertices)
{
    // CREATE VERTEX BUFFER
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Size of buffer (Size of one vertex * number of vertices)
    bufferInfo.size = sizeof(Vertex) * vertices->size();
    // Multiple types of buffer possible, we want Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    // Similar to Swap Chain images, can share vertex buffers
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    // Index of memory type on Physical Device that has required bit flags
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &vertexBufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate vertex buffer memory!");

    // Allocate memory to vertex buffer
    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // MAP MEMORY TO VERTEX BUFFER
    // 1. create pointer to a point in normal memory
    void* data;
    // 2. "Map" the vertex buffer memory to that point
    vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    // 3. Copy memory from vertices vector to the point
    memcpy(data, vertices->data(), (size_t)bufferInfo.size);
    // 4. Unmap the vertex buffer memory
    vkUnmapMemory(device, vertexBufferMemory);
}

uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        // Index of memory type must mach corresponding bit in allowedTypes && Desired property bit flags are part of memory type's property flags
        if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            // This memory type is valid, so return its index
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable memory type!");
}
