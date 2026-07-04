#pragma once

#include <fstream>

const int MAX_FRAME_DRAWS = 2;

const std::vector<const char*> deviceExtentions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

struct SwapChainDetails {
	// surface properties, e.g. image size/extent
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	// Surface image formats, e.g. RGBA and size of each color
	std::vector<VkSurfaceFormatKHR> formats;
	// How images should be presented to screen
	std::vector<VkPresentModeKHR> presentationModes;
};

struct SwapChainImage {
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> ReadFile(const std::string& filename)
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
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// Move read postion to the start of the file
	file.seekg(0);

	// Read the file date inot the buffer (stream "fileSize" in total
	file.read(fileBuffer.data(), fileSize);

	// Close stream
	file.close();

	return fileBuffer;
}