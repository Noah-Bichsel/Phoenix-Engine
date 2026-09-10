#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 20;

const std::vector<const char *> deviceExtentions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Vertex data representation
struct Vertex
{
    // Vertex Position (x, y, z)
    glm::vec3 pos;
    // Vertex Color (r, g, b)
    glm::vec3 col;
    // Texture Coords (u, v)
    glm::vec2 tex;
};

// Indices (Location) of Queue Families (if they exist at all)
struct QueueFamilyIndices
{
    // Location of Graphics Queue Family
    int graphicsFamily = -1;
    // Location of Presentation Queue Family
    int presentationFamily = -1;

    // Check if Queue Families are valid
    bool isValid()
    {
        return graphicsFamily >= 0 && presentationFamily >= 0;
    }
};

struct SwapChainDetails
{
    // surface properties, e.g. image size/extent
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    // Surface image formats, e.g. RGBA and size of each color
    std::vector<VkSurfaceFormatKHR> formats;
    // How images should be presented to screen
    std::vector<VkPresentModeKHR> presentationModes;
};

struct SwapChainImage
{
    VkImage image;
    VkImageView imageView;
};

static std::vector<char> ReadFile(const std::string &filename)
{
    // Open stream form given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if file stream succsessfully opend
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Get current read position and use to resize file buffer
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Move read postion to the start of the file
    file.seekg(0);

    // Read the file date inot the buffer (stream "fileSize" in total
    file.read(fileBuffer.data(), fileSize);

    // Close stream
    file.close();

    return fileBuffer;
}

static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes,
                                    VkMemoryPropertyFlags properties)
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

static void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize,
                         VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer *buffer,
                         VkDeviceMemory *bufferMemory)
{
    // CREATE VERTEX BUFFER
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Size of buffer (Size of one vertex * number of vertices)
    bufferInfo.size = bufferSize;
    // Multiple types of buffer possible
    bufferInfo.usage = bufferUsage;
    // Similar to Swap Chain images, can share vertex buffers
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    // Index of memory type on Physical Device that has required bit flags
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,
                                                          bufferProperties);

    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate vertex buffer memory!");

    // Allocate memory to vertex buffer
    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static VkCommandBuffer BeginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    // Command buffer to hold transfer command
    VkCommandBuffer commandBuffer;

    // Command buffer details
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Information to begin command buffer record
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // We're only using the command buffer once, so set up for one time submit
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Begin recording transfer commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void EndAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                                      VkCommandBuffer commandBuffer)
{
    // End commands
    vkEndCommandBuffer(commandBuffer);

    // Queue Submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // Free temporary command buffer back to pool
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// INFO: in the way it is set up it is not good for tones of meshes being loaded should add synchronization to be able to load models at the same time
static void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer,
                       VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
    // Create Buffer
    VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(device, transferCommandPool);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    // Command to copy src buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

    EndAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                            VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
{
    // Create Buffer
    VkCommandBuffer transferCommandBuffer = BeginCommandBuffer(device, transferCommandPool);

    VkBufferImageCopy imageRegion = {};
    // Offset into data
    imageRegion.bufferOffset = 0;
    // Row length of data to calculate data spacing
    imageRegion.bufferRowLength = 0;
    // Image height to calculate data spacing
    imageRegion.bufferImageHeight = 0;
    // Which aspect of image to copy
    imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // MipMap level to copy
    imageRegion.imageSubresource.mipLevel = 0;
    // Starting array layer (if array)
    imageRegion.imageSubresource.baseArrayLayer = 0;
    // Number of layers to copy starting at base array layer
    imageRegion.imageSubresource.layerCount = 1;
    // Offset into image (as opposed to raw data in buffer offset
    imageRegion.imageOffset = {0, 0, 0};
    // Size of region to copy as (x, y, z) values
    imageRegion.imageExtent = {width, height, 1};

    // Copy Buffer to given image
    vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &imageRegion);

    EndAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image,
                                  VkImageLayout oldLayout, VkImageLayout newLayout)
{
    // Create Buffer
    VkCommandBuffer commandBuffer = BeginCommandBuffer(device, commandPool);

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // Layout to transition from
    imageMemoryBarrier.oldLayout = oldLayout;
    // layout to transition to
    imageMemoryBarrier.newLayout = newLayout;
    // Queue family to transition from
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // Queue family to transition to
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // Image being accessed and modified as part of barrier
    imageMemoryBarrier.image = image;
    // Aspect of image being altered
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // First mip level to start alterations on
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    // Number of Mip levels to alter starting from baseMipLevel
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    // First layer to start alteration on
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    // Number of layers to alter starting from baseArrayLayer
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    // If transitioning form new image to image ready to receive data...
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        // Memory access stage transition must after...
        imageMemoryBarrier.srcAccessMask = 0;
        // Memory access stage transition must before...
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    // If transiting form transfer destination to shader readable...
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        // Pipeline stages (match to src and dst AccessMask)
        srcStage, dstStage,
        // Dependency flags
        0,
        // Memory Barrier count + data
        0, nullptr,
        // Buffer Memory Barrier count + data
        0, nullptr,
        // Image Memory Barrier count + data
        1, &imageMemoryBarrier
    );

    EndAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}
