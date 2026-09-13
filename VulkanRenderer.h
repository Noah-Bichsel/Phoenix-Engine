#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "stb_image.h"

#include "Mesh.h"
#include "MeshModel.h"
#include "Utilites.h"
#include "VulkanValidation.h"

class VulkanRenderer
{
public:
	VulkanRenderer();

	int init(GLFWwindow* newWindow);

	int CreateMeshModel(std::string modelFile);
	void updateModel(int modelId, glm::mat4 newModel);

	void Draw();
	void cleanup();

private:
	GLFWwindow* window;

	int currentFrame = 0;

	// Scene Objects
	std::vector<MeshModel> modelList;

	// Scene Settings
	// MVP -> model view projection
	struct UboViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	// Vulkan components
	// - Main
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	struct 
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;

	std::vector<SwapChainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkImage> colorBufferImage;
	std::vector<VkDeviceMemory> colorBufferImageMemory;
	std::vector<VkImageView> colorBufferImageView;

	std::vector<VkImage> depthBufferImage;
	std::vector<VkDeviceMemory> depthBufferImageMemory;
	std::vector<VkImageView> depthBufferImageView;
	VkFormat depthFormat;

	VkSampler textureSampler;

	// - Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSetLayout inputSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	VkDescriptorPool inputDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;
	std::vector<VkDescriptorSet> inputDescriptorSets;

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	std::vector<VkBuffer> modelDUniformBuffer;
	std::vector<VkDeviceMemory> modelDUniformBufferMemory;

	//VkDeviceSize minUniformBufferOffset;
	//size_t modelUniformAlignment;
	//UboModel* modelTransferSpace;

	// - Assets
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// - Pipeline
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	VkPipeline secondPipline;
	VkPipelineLayout secondPiplineLayout;

	VkRenderPass renderPass;

	// - Pools
	VkCommandPool graphicsCommandPool;

	// - Utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// - Synchronisation
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	// Vulkan functions
	// - Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateDebugCallback();
	void CreateSurface();
	void CreateSwapChain();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreatePushConstantRange();
	void CreateGraphicsPipeline();
	void CreateColorBufferImage();
	void CreateDepthBufferImage();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncronization();
	void CreateTextureSampler();

	void CreateUniformBuffer();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateInputDescriptorSets();

	void UpdateUniformBuffers(uint32_t imageIndex);

	// - Record Functions
	void RecordCommands(uint32_t currentImage);

	// - Get functions
	void GetPhysicalDevice();

	// - Allocate Functions
	void AllocateDynamicBufferTransferSpace();
	
	// - Support functions
	// -- Checker Functions
	bool CheckInstanceExtentionSupport(std::vector<const char*>* checkExtentions);
	bool CheckDeviceExtentionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkValidationLayerSupport();

	// -- Getter Functions
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice device);

	// -- Choose Functions
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat ChooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featuresFlags);

	// -- Create Functions
	VkImage CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usaFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	int CreateTextureImage(std::string filename);
	int CreateTexture(std::string filename);
	int CreateTextureDescriptor(VkImageView textureImage);

	// -- Loader Functions
	stbi_uc* LoadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
};

