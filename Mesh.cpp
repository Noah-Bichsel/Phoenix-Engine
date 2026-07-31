#include "Mesh.h"

#include <cstring>

Mesh::Mesh() {}

Mesh::Mesh(VkPhysicalDevice new_physical_device, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> *vertices, std::vector<uint32_t>* indices)
: indexCount(indices->size()), vertexCount(vertices->size()), physicalDevice(new_physical_device), device(newDevice)
{
    CreateVertexBuffer(transferQueue, transferCommandPool, vertices);
    CreateIndexBuffer(transferQueue, transferCommandPool, indices);
}

int Mesh::GetVertexCount()
{
    return vertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
    return vertexBuffer;
}

int Mesh::GetIndexCount()
{
    return indexCount;
}

VkBuffer Mesh::GetIndexBuffer()
{
    return indexBuffer;
}

void Mesh::DestroyBuffers()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
}

Mesh::~Mesh()
{

}

void Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> *vertices)
{
    // Get size of buffer needed for vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Create Staging Buffer and Allocate Memory to it
    CreateBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO VERTEX BUFFER
    // 1. create pointer to a point in normal memory
    void* data;
    // 2. "Map" the vertex buffer memory to that point
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    // 3. Copy memory from vertices vector to the point
    memcpy(data, vertices->data(), (size_t)bufferSize);
    // 4. Unmap the vertex buffer memory
    vkUnmapMemory(device, stagingBufferMemory);

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer Memory is to be DEVICE_LOCAL_BIT meaning memory is the GPU and only accessible by it and not CPU (host)
    CreateBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

    // Copy staging buffer to vertex buffer on GPU
    CopyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

    // Clean up staging buffer parts
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices)
{
    // Get size of buffer needed for indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO INDEX BUFFER
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices->data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create buffer for INDEX data on GPU access only area
    CreateBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

    // Copy from staging buffer to GPU access buffer
    CopyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

    // Destroy + Release Staging Buffer resources
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
