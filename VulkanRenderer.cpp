#include "VulkanRenderer.h"
#include <cstring>
#include <limits>

#include "assimp/postprocess.h"

VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow *newWindow)
{
    window = newWindow;

    try
    {
        // needs to be in this order
        CreateInstance();
        CreateDebugCallback();
        CreateSurface(); 
        GetPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateDepthBufferImage();
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreatePushConstantRange();
        CreateGraphicsPipeline();
        CreateColorBufferImage();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateTextureSampler();
        //AllocateDynamicBufferTransferSpace();
        CreateUniformBuffer();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateInputDescriptorSets();
        CreateSyncronization();

        uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.0f);
        uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 110.0f), glm::vec3(0.0f, 0.0f, .0f), glm::vec3(0.0f, 1.0f, 0.0f));

        uboViewProjection.projection[1][1] *= -1.0f;

        // Create our default "no texture" texture
        CreateTexture("DefaultTexture.png");
    }
    catch (const std::runtime_error &e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel)
{
    if (modelId >= modelList.size())
        return;

    modelList[modelId].SetModel(newModel);
}

void VulkanRenderer::Draw()
{
    // -- GET NEXT IMAGE --
    // Wait for given fence to signal (open) from last draw before continuing
    vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mainDevice.logicalDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

    RecordCommands(imageIndex);
    UpdateUniformBuffers(imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue Submission information
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // Number of semaphores to wait on
    submitInfo.waitSemaphoreCount = 1;
    // List of semaphores to wait on
    submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    // Stages to check semaphores at
    submitInfo.pWaitDstStageMask = waitStages;
    // Number of command buffers to submit
    submitInfo.commandBufferCount = 1;
    // Command buffer to submit
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    // Number of semaphores to signal
    submitInfo.signalSemaphoreCount = 1;
    // Semaphores to signal when command buffer finishes
    submitInfo.pSignalSemaphores = &renderFinished[currentFrame];

    // Submit command buffer to the queue
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit command buffer to Queue!");
    }

    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Number of semaphores to wait on
    presentInfo.waitSemaphoreCount = 1;
    // semaphores to wait on
    presentInfo.pWaitSemaphores = &renderFinished[currentFrame];
    // Number of swapchians to present to
    presentInfo.swapchainCount = 1;
    // swapchians to present images to
    presentInfo.pSwapchains = &swapChain;
    // Index of images in swapchains to present
    presentInfo.pImageIndices = &imageIndex;

    // Present Image
    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR ) // REDO TO /* && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR */
    {
        throw std::runtime_error("Failed to present Image!");
    }

    // Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
    // Wait until no actions being run on device
    vkDeviceWaitIdle(mainDevice.logicalDevice);

    /*
#if defined(__linux__)
    free(modelTransferSpace);
#else
    _aligned_free(modelTransferSpace);
#endif
    */

    for (size_t i = 0; i < modelList.size(); i++)
    {
        modelList[i].DestroyMeshModel();
    }

    vkDestroyDescriptorPool(mainDevice.logicalDevice, inputDescriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, inputSetLayout, nullptr);

    vkDestroyDescriptorPool(mainDevice.logicalDevice, samplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, samplerSetLayout, nullptr);

    vkDestroySampler(mainDevice.logicalDevice, textureSampler, nullptr);

    for (size_t i = 0; i < textureImages.size(); i++)
    {
        vkDestroyImageView(mainDevice.logicalDevice, textureImageViews[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, textureImages[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, textureImageMemory[i], nullptr);
    }

    for (size_t i = 0; i < depthBufferImage.size(); i++)
    {
        vkDestroyImageView(mainDevice.logicalDevice, depthBufferImageView[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, depthBufferImage[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, depthBufferImageMemory[i], nullptr);
    }

    for (size_t i = 0; i < colorBufferImage.size(); i++)
    {
        vkDestroyImageView(mainDevice.logicalDevice, colorBufferImageView[i], nullptr);
        vkDestroyImage(mainDevice.logicalDevice, colorBufferImage[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, colorBufferImageMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
        vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);

        //vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
        //vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
    }
    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
    for (auto framebuffer: swapChainFramebuffers)
        vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
    vkDestroyPipeline(mainDevice.logicalDevice, secondPipline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, secondPiplineLayout, nullptr);
    vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);
    for (auto image: swapChainImages)
        vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
    vkDestroySwapchainKHR(mainDevice.logicalDevice, swapChain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    vkDestroyDevice(mainDevice.logicalDevice, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::CreateInstance()
{
    if (!checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Information about the application itself
    // Most data here does not effect the program and is for developer convinance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // custom name of the application
    appInfo.pApplicationName = "Vulkan App";
    // custom version of the application VK_MAKE_VERSION(Major, Miner, Patch)
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    // custom name of the engine, if you are not using an engine you can just put "No Engine"
    appInfo.pEngineName = "No Engine";
    // custom version of the engine VK_MAKE_VERSION(Major, Miner, Patch)
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // Vulkan API version - DOES EFFECT APPLICATION
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Creation Infromation for a VkInstance (Vulkan Instance)
    VkInstanceCreateInfo createInfo{};
    // sType = structure type, tells Vulkan what type of structure we are using, this is required for all Vulkan structures
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Debug messenger creation info, needs to be passed to instance create info so that validation layers can use it to report issues during instance creation (and destruction)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (enableValidationLayers)
    {
        // Set up validation layers that Instance will use
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    // Create list hold instance extensions
    std::vector<const char *> instanceExtentions = std::vector<const char *>();

    // Set up extentions Instance will use
    // GLFW may require multiple Extentions
    uint32_t glfwExtensionCount = 0;
    // Extentions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)
    const char **glfwExtensions;

    // get GLFW extentions
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extentions to list of extentions
    for (size_t i = 0; i < glfwExtensionCount; i++)
        instanceExtentions.push_back(glfwExtensions[i]);

    // Add validation layer extention
    instanceExtentions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Check Instance Extentions Supported...
    if (!CheckInstanceExtentionSupport(&instanceExtentions))
    {
        throw std::runtime_error("VkInstance does not support required extentions!");
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtentions.size());
    createInfo.ppEnabledExtensionNames = instanceExtentions.data();

    // Create the Vulkan Instance
    VkResult restult = vkCreateInstance(&createInfo, nullptr, &instance);

    if (restult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan Instance");
    }
}

void VulkanRenderer::CreateLogicalDevice()
{
    // Get the queue family indices for the chosen physical device
    QueueFamilyIndices indices = GetQueueFamilies(mainDevice.physicalDevice);

    // Vector for queue creation information, and set for family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

    // Queues the logical device needs to create and info to do so
    for (int queueFamilyIndex: queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // the index of the family to create a queue from
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        // number of queues to create
        queueCreateInfo.queueCount = 1;
        float priority = 1.0f;
        // vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // number of queue create infos
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    // List of queue create infos so device can create required queues
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    // Number of enabled logical device extensions
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtentions.size());
    // List of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtentions.data();

    // Physical Device Features that the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures{};
    // enabling anisotropy
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    //deviceFeatures.depthClamp = VK_TRUE;

    // Physical Device Features Logical Device will be using
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // Create the Logical Device for the Physical Device
    VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Logical Device!");
    }

    // Queues are created at the same time as the device...
    // So we want handle to queues
    // from given logical device, of given Queue Family Index (0 since only one queue), place reference in given vkQueue
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::CreateDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanRenderer::CreateSurface()
{
    // Sreate Surface (creates a surace create info struct, runs the create surface funtion, resturns result)
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void VulkanRenderer::CreateSwapChain()
{
    // Get Swape Chain Details so we can pick best settings
    SwapChainDetails swapChainDetails = GetSwapChainDetails(mainDevice.physicalDevice);

    // Find optimal surface values for our swapchain
    VkSurfaceFormatKHR surfaceFormat = ChooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentationMode = ChooseBestPresentationMode(swapChainDetails.presentationModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are are in the swapchain? Get 1 more then the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If imageCount higher than max, then clamp down max
    // If 0 than limatless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    // Create information for swapchain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // swapchain surface
    swapChainCreateInfo.surface = surface;
    // swapchain format
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    // swapchain color space
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    // swapchain presentaion mode
    swapChainCreateInfo.presentMode = presentationMode;
    // swapchain image extents
    swapChainCreateInfo.imageExtent = extent;
    // Minimum images in swapechian
    swapChainCreateInfo.minImageCount = imageCount;
    // Number of layers for each image in chain
    swapChainCreateInfo.imageArrayLayers = 1;
    // What attachment images will be used as
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // Transform to preform on swapchain images
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
    // How to handle blending images with external graphics (e.g. other windows)
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Wether to clip parts of images not in view (e.g. behind other windows, off screen, etc)
    swapChainCreateInfo.clipped = VK_TRUE;

    // Get Queue Family Indices
    QueueFamilyIndices indices = GetQueueFamilies(mainDevice.physicalDevice);

    // If graphics and presentation queues are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
            (uint32_t) indices.graphicsFamily,
            (uint32_t) indices.presentationFamily
        };

        // Image Share handling
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // Number of queues to share images
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        // Array of queues to share between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // If old swapchain been destoryed and this one replaces it, then link old one to quickly handover responsabilities
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create swapchain
    VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapChain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swapchain!");
    }

    // Store for later reference
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    // Get swapchain images (first count, then values)
    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChain, &swapChainImageCount, nullptr);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChain, &swapChainImageCount, images.data());

    for (VkImage image: images)
    {
        // store image handle
        SwapChainImage swapChainImage = {};
        swapChainImage.image = image;
        swapChainImage.imageView = CreateImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Adds to swapchain image list
        swapChainImages.push_back(swapChainImage);
    }
}

void VulkanRenderer::CreateRenderPass()
{
    // Array of subpasses
    std::array<VkSubpassDescription, 2> subpasses{};

    // ATTACHMENTS
    // SUBPASS 1 ATTACHMENTS + REFRENCES (INPUT ATTACHMENT)

    // Color Attachment (Input)
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = ChooseSupportedFormat(
        { VK_FORMAT_R8G8B8A8_UNORM },
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment (Input)
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Color Attachment (Input) Reference
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 1;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment (input) Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 2;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set up Subpass 1
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorAttachmentReference;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

    // SUBPASS 2 ATTACHMENT + REFERENCES

    // Swap chain color attachment
    VkAttachmentDescription swapChainColorAttachment = {};
    // Format to use for attachment
    swapChainColorAttachment.format = swapChainImageFormat;
    // Number of samples to write for multisampling
    swapChainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Originally: VK_SAMPLE_COUNT_16_BIT
    // Describes what to do with attachment before rendering
    swapChainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // Describes what to do with attachment after rendering
    swapChainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Describes what to do with stencil for rendering
    swapChainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // Describes what to do with stencil after rendering
    swapChainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Framebuffer data will be stored as an image, but images can be given different data layouts to give optimal use for certain operations
    // Image data layout before render pass starts
    swapChainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Image data layout after render pass (to change to)
    swapChainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference swapChainColorAttachmentReference = {};
    swapChainColorAttachmentReference.attachment = 0;
    swapChainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // References to attachments that subpass will take input from
    std::array<VkAttachmentReference, 2> inputReferences = {};
    inputReferences[0].attachment = 1;
    inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputReferences[1].attachment = 2;
    inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Set up Subpass 2
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &swapChainColorAttachmentReference;
    subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
    subpasses[1].pInputAttachments = inputReferences.data();

    // SUBPASS DEPENDENCY

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 3> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    //
    // Subpass index (Vk_SUBPASS_EXTERNAL = special value meaning out side of render pass)
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    // Pipeline stage
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    // Stage access mask (memory access)
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    // But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;

    // Subpass 1 layout (color/depth) to Subpass 2 layout (shader read)
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    subpassDependencies[1].dstSubpass = 1;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[2].srcSubpass = 0;
    subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // But must happen before...
    subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[2].dependencyFlags = 0;

    std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapChainColorAttachment, colorAttachment, depthAttachment };

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // UboViewProjection Binding Info
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    // Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.binding = 0;
    // Type of descriptor (uniform, dynamic uniform, image sampler, ect)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // Number of descriptors for binding
    vpLayoutBinding.descriptorCount = 1;
    // Shader stage to bind to
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // For Textures: can make sampler data unchangeable (immutable) by specifying in layout
    vpLayoutBinding.pImmutableSamplers = nullptr;

    // Model Binding Info
    /*VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;
    */

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

    // Create Descriptor Set Layout with given binding
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // Number of binding infos
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    // Array of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();

    // Create Descriptor Set Layout
    VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding. descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor Set Layout
    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr, &samplerSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    // CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
    // Color Input Binding
    VkDescriptorSetLayoutBinding colorInputLayoutBinding = {};
    colorInputLayoutBinding.binding = 0;
    colorInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputLayoutBinding.descriptorCount = 1;
    colorInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Depth Input Binding
    VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
    depthInputLayoutBinding.binding = 1;
    depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputLayoutBinding.descriptorCount = 1;
    depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Array of input attachment bindings
    std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colorInputLayoutBinding , depthInputLayoutBinding };

    // Create a Descriptor set layout for input attachments
    VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
    inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
    inputLayoutCreateInfo.pBindings = inputBindings.data();

    // Create Descriptor Set Layout
    result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &inputLayoutCreateInfo, nullptr, &inputSetLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void VulkanRenderer::CreatePushConstantRange()
{
    // Define push constant values (no 'create' needed!)
    // Shader stage push constant will go to
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // Offset into given data to pass to push constant
    pushConstantRange.offset = 0;
    // size of data being passed
    pushConstantRange.size = sizeof(Model);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
    // Read in SPER-V code of shaders
    auto vertexShaderCode = ReadFile("Shaders/vert.spv");
    auto fragmentShaderCode = ReadFile("Shaders/frag.spv");

    // Create Shader Modules
    VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

    // -- SHADER STAGE CREATION INFORMATION
    // Vertex Shader Stage Creation Information
    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Shader Stage name
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Shader module to be used by stage
    vertexShaderStageCreateInfo.module = vertexShaderModule;
    // Entry point in to shader
    vertexShaderStageCreateInfo.pName = "main";

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Shader Stage name
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Shader module to be used by stage
    fragmentShaderStageCreateInfo.module = fragmentShaderModule;
    // Entry point in to shader
    fragmentShaderStageCreateInfo.pName = "main";

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires arry of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    // How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    // Can bind multiple streams of data, this defines which one
    bindingDescription.binding = 0;
    // Size of a single vertex object
    bindingDescription.stride = sizeof(Vertex);
    // How to move between data after each vertex
    // VK_VERTEX_INPUT_RATE_VERTEX      :   Move onto the next vertex
    // VK_VERTEX_INPUT_RATE_INSTANCE    :   Move to a vertex for the next instance
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // How the data of an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

    // Position Attribute
    // Witch binding the data is at (should be same as above)
    attributeDescriptions[0].binding = 0;
    // Location in shader where data will be read from
    attributeDescriptions[0].location = 0;
    // Format the data will take (also helps define size of data)
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    // Where this attribute is defined in the data for a single vertex
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // Color attributes
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, col);

    // Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);

    // -- VERTEX INPUT  --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    // List of vertex Binding Descripts (data spacing/stride informaion)
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    // List of vertex Attribute Desritions (data format and where to bind to/from)
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // Primitive type to assemble vertices as
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Allow overriding of "strip" topology to start new primitives
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // -- VIEWPORT & SCISSOR --
    // for things like split screen

    //Create a viewport info struct
    VkViewport viewPort = {};
    // x start coordinate
    viewPort.x = 0.0f;
    // y start coordinate
    viewPort.y = 0.0f;
    // width of viewport
    viewPort.width = (float) swapChainExtent.width;
    // height of viewport
    viewPort.height = (float) swapChainExtent.height;
    // min framebuffer depth
    viewPort.minDepth = 0.0f;
    // max framebuffer depth
    viewPort.maxDepth = 1.0f;

    // Create a scissor info struct
    VkRect2D scissor = {};
    // Offset to use reagon from
    scissor.offset = {0, 0};
    // Exten to describe reagon to use, starting at offset
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewPort;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    /*
    // -- DYNAMIC STATES --
    // Dynamic states to enable
    std::vector<VkDynamicState> dynamicStateEnable;
    // Dynmamic Viewport : Can resisze in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    // Dynamic Scissor : Can resisze in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    dynamicStateEnable.push_back(VK_DYNAMIC_STATE_SCISSOR);

    // Dynamic State creation info
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnable.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStateEnable.data();
    */

    // -- RASTERIZER --
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    // How to handle filling points between vertices
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // How thick lines should be when drawn
    rasterizerCreateInfo.lineWidth = 1.0f;
    // Which face of a tri to cull
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    // Winding to determine which side is front
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Whether to add depth bias to fragments (good for stopping "shadow acne" when using shadow maps)
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisample shading or not (for antialiasing, etc)
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    // Number of samples to use per fragment
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // -- BLENDING --
    // Blending decides how to blend a new color being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colorState = {};
    // Colors to apply blending to
    colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
    // Enable Blending
    colorState.blendEnable = VK_TRUE;

    // Blending uses equation: (srcColorBlendFactor * newColor) colorBlendOp (dstColorBlendFactor * oldColor)
    colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorState.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColor)
    //			   (newColorAlpah * newColor) + ((1 - newColorAlpha) * oldColor)

    colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorState.alphaBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (1 * newAlpha) + (0 * oldAlpha) = newAlpha

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Alternative to calaulation is to use logical operations
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;


    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Alternative to calculation is to use logical operations
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorState;

    // -- PIPELINE LAYOUT --
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr,
                                             &pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // Enable checking depth to determine fragment write
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    // Enable writing to depth buffer (to repace old values)
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    // Comparison operation that allows an overwrite (is in front)
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // Depth Bound Test: does the depth value exist between 2 bounds
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    // Enable Stencil Test
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;


    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // Number of shader stages
    pipelineCreateInfo.stageCount = 2;
    // List of shader stages
    pipelineCreateInfo.pStages = shaderStages;
    // All the fixed function
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    // Pipeline layout pipeline should use
    pipelineCreateInfo.layout = pipelineLayout;
    // Render pass description the pipeline is compatible with
    pipelineCreateInfo.renderPass = renderPass;
    // Subpass of render pass to use with pipeline
    pipelineCreateInfo.subpass = 0;

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
    // Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    // or index of pipeline being created to derive from (in case creating multiple pipelines at once)
    pipelineCreateInfo.basePipelineIndex = -1;

    // Create Graphics Pipeline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                       &graphicsPipeline);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // Destroy Shader Modules, no longer needed after pipeline creation
    vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);


    // CREATE SECOND PASS PIPELINE
    // Second pass shaders
    auto secondVertexShaderCode = ReadFile("Shaders/second_vert.spv");
    auto secondFragmentShaderCode = ReadFile("Shaders/second_frag.spv");

    // Build shaders
    VkShaderModule secondVertexShaderModule = CreateShaderModule(secondVertexShaderCode);
    VkShaderModule secondFragmentShaderModule = CreateShaderModule(secondFragmentShaderCode);

    // Set new shaders
    vertexShaderStageCreateInfo.module = secondVertexShaderModule;
    fragmentShaderStageCreateInfo.module = secondFragmentShaderModule;

    VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

    // No vertex data for second pass
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Dont want to write to depth buffer
    depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

    // Create new pipeline layout
    VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
    secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    secondPipelineLayoutCreateInfo.setLayoutCount = 1;
    secondPipelineLayoutCreateInfo.pSetLayouts = &inputSetLayout;
    secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(mainDevice.logicalDevice, &secondPipelineLayoutCreateInfo, nullptr, &secondPiplineLayout);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    // Update second shader stage list
    pipelineCreateInfo.pStages = secondShaderStages;
    // Change pipeline layout for input attachment descriptor sets
    pipelineCreateInfo.layout = secondPiplineLayout;
    // Use second subpass
    pipelineCreateInfo.subpass = 1;

    // Create second pipline
    result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &secondPipline);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Graphics Pipeline!");

    // Destroy second shader modules
    vkDestroyShaderModule(mainDevice.logicalDevice, secondFragmentShaderModule, nullptr);
    vkDestroyShaderModule(mainDevice.logicalDevice, secondVertexShaderModule, nullptr);
}

void VulkanRenderer::CreateColorBufferImage()
{
    // Resize supported format for color attachment
    colorBufferImage.resize(swapChainImages.size());
    colorBufferImageMemory.resize(swapChainImages.size());
    colorBufferImageView.resize(swapChainImages.size());

    // Get supported format for color attachment
    VkFormat colorFormat = ChooseSupportedFormat(
        { VK_FORMAT_R8G8B8A8_UNORM },
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        // Create Color Buffer Image
        colorBufferImage[i] = CreateImage(swapChainExtent.width, swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &colorBufferImageMemory[i]);

        // Create Color Buffer Image View
        colorBufferImageView[i] = CreateImageView(colorBufferImage[i], colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::CreateDepthBufferImage()
{
    depthBufferImage.resize(swapChainImages.size());
    depthBufferImageMemory.resize(swapChainImages.size());
    depthBufferImageView.resize(swapChainImages.size());

    // Get supported format for depth buffer
    depthFormat = ChooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        // Create Depth Buffer Image
        depthBufferImage[i] = CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory[i]);

        // Create Depth Buffer Image View
        depthBufferImageView[i] = CreateImageView(depthBufferImage[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

void VulkanRenderer::CreateFramebuffers()
{
    // Resize frame buffer count to equal swap chian image count
    swapChainFramebuffers.resize(swapChainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        std::array<VkImageView, 3> attachments
        {
            swapChainImages[i].imageView,
            colorBufferImageView[i],
            depthBufferImageView[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // Render pass layout the Framebuffer will be used with
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        // List of attachments (1:1 with image pass)
        framebufferCreateInfo.pAttachments = attachments.data();
        // Frame buffer width
        framebufferCreateInfo.width = swapChainExtent.width;
        // Frame buffer height
        framebufferCreateInfo.height = swapChainExtent.height;
        // Frame buffer layers
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr,
                                              &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void VulkanRenderer::CreateCommandPool()
{
    // Get indices of queue families form device
    QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // Queue Family type that buffers from this command pool will use
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    // Create a Graphics queue Family Command pool
    VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
}

void VulkanRenderer::CreateCommandBuffers()
{
    // Resize command buffer count to have one for each frame buffer
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool;
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY   : Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer cant be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void VulkanRenderer::CreateSyncronization()
{
    imageAvailable.resize(MAX_FRAME_DRAWS);
    renderFinished.resize(MAX_FRAME_DRAWS);
    drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore Creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i  = 0; i < MAX_FRAME_DRAWS; i++)
    {
        if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphore and/or Fence!");
        }
    }
}

void VulkanRenderer::CreateTextureSampler()
{
    // Sampler Create Infor
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // How to render when image is magnified on screen
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    // How to render when image is minimized on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    // How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // How to handle texture wrap in W (Z) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // Border beyond texture (Only works for border clamp)
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Whether coords should be normalized (between 0 and 1)
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    // Mipmap interpolation mode
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // Level of detail bias for mip level
    samplerCreateInfo.mipLodBias = 0.0f;
    // Minimum level of detail to pick mip level
    samplerCreateInfo.minLod = 0.0f;
    // Maximum level of detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;
    // Enable Anisotropy
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    // Anisotropy sample Level
    samplerCreateInfo.maxAnisotropy = 16;

    VkResult result = vkCreateSampler(mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}

void VulkanRenderer::CreateUniformBuffer()
{
    // ViewProject buffer size
    VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (andy by extension command buffer)
    vpUniformBuffer.resize(swapChainImages.size());
    vpUniformBufferMemory.resize(swapChainImages.size());

    //modelDUniformBuffer.resize(swapChainImages.size());
    //modelDUniformBufferMemory.resize(swapChainImages.size());

    // Create uniform buffers
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        CreateBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

        //CreateBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);
    }
}

void VulkanRenderer::CreateDescriptorPool()
{
    // CREATE UNIFORM DESCRIPTOR POOL

    // Type of Descriptors and how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
    // ViewProjection Pool
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBufferMemory.size());

    // Model Pool (DYNAMIC)
    /*VkDescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBufferMemory.size());
    */

    // List of pool sizes
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {vpPoolSize};

    // Data to create Descriptor Pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // Maximum number of descriptor sets that can be created form pool
    poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    // Amount of Pool Sizes being passed
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    // Pool Sizes to create pool with
    poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

    // Create Descriptor Pool
    VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");

    // CREATE SAMPLER DESCRIPTOR POOL
    // Texture sampler pool
    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = MAX_OBJECTS;

    VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
    samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
    samplerPoolCreateInfo.poolSizeCount = 1;
    samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

    result = vkCreateDescriptorPool(mainDevice.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");

    // CREATE INPUT ATTACHMENT DESCRIPTOR POOL
    // Color Attachment Pool Size
    VkDescriptorPoolSize colorInputPoolSize = {};
    colorInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    colorInputPoolSize.descriptorCount = static_cast<uint32_t>(colorBufferImageView.size());

    // Depth Attachment Pool Size
    VkDescriptorPoolSize depthInputPoolSize = {};
    depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputPoolSize.descriptorCount = static_cast<uint32_t>(depthBufferImageView.size());

    std::vector<VkDescriptorPoolSize> inputPoolSizes = { colorInputPoolSize, depthInputPoolSize };

    // Create input attachment pool
    VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
    inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    inputPoolCreateInfo.maxSets = swapChainImages.size();
    inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
    inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

    result = vkCreateDescriptorPool(mainDevice.logicalDevice, &inputPoolCreateInfo, nullptr, &inputDescriptorPool);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void VulkanRenderer::CreateDescriptorSets()
{
    // Resize Descriptor Set list so one for every buffer
    descriptorSets.resize(swapChainImages.size());

    std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), descriptorSetLayout);

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // Pool to allocate Descriptor Set from
    setAllocInfo.descriptorPool = descriptorPool;
    // Number of sets to allocate
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    // Layout to use to allocate sets (1:1 relationship)
    setAllocInfo.pSetLayouts = setLayouts.data();

    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, descriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        // VIEW PROJECTION DESCRIPTOR
        // Buffer info and data offset info
        VkDescriptorBufferInfo mvpBufferInfo = {};
        // Buffer to get data from
        mvpBufferInfo.buffer = vpUniformBuffer[i];
        // Position of start of data
        mvpBufferInfo.offset = 0;
        // Size of data
        mvpBufferInfo.range = sizeof(UboViewProjection);

        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // Descriptor Set to update
        vpSetWrite.dstSet = descriptorSets[i];
        // Binding to update (matches with binding with layout/shader)
        vpSetWrite.dstBinding = 0;
        // Index array to update
        vpSetWrite.dstArrayElement = 0;
        // Type of descriptor
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        // Amount to update
        vpSetWrite.descriptorCount = 1;
        // Information about buffer data to bind
        vpSetWrite.pBufferInfo = &mvpBufferInfo;

        // MODEL DESCRIPTOR
        // Model Buffer Binding Info
        /*VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelDUniformBuffer[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = modelUniformAlignment;

        VkWriteDescriptorSet modelSetWrite = {};
        modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelSetWrite.dstSet = descriptorSets[i];
        modelSetWrite.dstBinding = 1;
        modelSetWrite.dstArrayElement = 0;
        modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelSetWrite.descriptorCount = 1;
        modelSetWrite.pBufferInfo = &modelBufferInfo;
        */

        // List of Descriptor Set Writes
        std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);

    }

}

void VulkanRenderer::CreateInputDescriptorSets()
{
    // Resize array to hold descriptor set for each swap chain image
    inputDescriptorSets.resize(swapChainImages.size());

    // Fill array for layouts ready for set creation
    std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), inputSetLayout);

    // Input Attachment Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = inputDescriptorPool;
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    setAllocInfo.pSetLayouts = setLayouts.data();

    // Allocate Descriptor Sets
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, inputDescriptorSets.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate Input Attachment descriptor sets!");

    // Update each descriptor set with input attachment
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        // Color Attachment Descriptor
        VkDescriptorImageInfo colorAttachmentDescriptor = {};
        colorAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorAttachmentDescriptor.imageView = colorBufferImageView[i];
        colorAttachmentDescriptor.sampler = VK_NULL_HANDLE;

        // Color Attachment Descriptor Write
        VkWriteDescriptorSet colorWrite = {};
        colorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorWrite.dstSet = inputDescriptorSets[i];
        colorWrite.dstBinding = 0;
        colorWrite.dstArrayElement = 0;
        colorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        colorWrite.descriptorCount = 1;
        colorWrite.pImageInfo = &colorAttachmentDescriptor;

        // Depth Attachment Descriptor
        VkDescriptorImageInfo depthAttachmentDescriptor = {};
        depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthAttachmentDescriptor.imageView = depthBufferImageView[i];
        depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

        // Depth Attachment Descriptor Write
        VkWriteDescriptorSet depthWrite = {};
        depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depthWrite.dstSet = inputDescriptorSets[i];
        depthWrite.dstBinding = 1;
        depthWrite.dstArrayElement = 0;
        depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        depthWrite.descriptorCount = 1;
        depthWrite.pImageInfo = &depthAttachmentDescriptor;

        // List of input descriptor set writes
        std::vector<VkWriteDescriptorSet> setWrites = { colorWrite, depthWrite };

        // Update descriptor Sets
        vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
    }

}

void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
    // Copy VP data
    void* data;
    vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
    memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
    vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);

    // Copy Model data
    /*for (size_t i = 0; i < meshList.size(); i++)
    {
        UboModel* thisModel = (UboModel*)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
        *thisModel = meshList[i].GetModel();
    }

    // Map the list of model data
    vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
    memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
    vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);
    */
}

void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // Information abot how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // Render pass to begin
    renderPassBeginInfo.renderPass = renderPass;
    // Start of point of render pass in pixels
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    // Size of region to run render pass on (starting at offset)
    renderPassBeginInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 3> clearValues = {};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    // Background color
    clearValues[1].color = {0.6f, 0.65f, 0.04f, 1.0f};
    clearValues[2].depthStencil.depth = 1.0f;

    renderPassBeginInfo.pClearValues = clearValues.data();
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

    renderPassBeginInfo.framebuffer = swapChainFramebuffers[currentImage];

    // Start recording commands to command buffer
    VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer!");

        // Begin Render Pass
        vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Bind Pipeline to be used in render pass
            vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            for (size_t j = 0; j < modelList.size(); j++)
            {
                MeshModel thisModel = modelList[j];
                // to fix referencing problem
                glm::mat4 m = thisModel.GetModel();

                // "Push" constants to given shader stage directly (no buffer)
                vkCmdPushConstants(commandBuffers[currentImage],
                    pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT,     // Stage to push constants to
                    0,                              // Offset of push constants to update
                    sizeof(Model),                  // size of data being pushed
                    &m                              // Actual data being pushed (can be array)
                );

                for (size_t k = 0; k < thisModel.GetMeshCount(); k++)
                {
                    // Buffers to bind
                    VkBuffer vertexBuffer[] = { thisModel.GetMesh(k)->GetVertexBuffer() };
                    // Offsets into buffers being bound
                    VkDeviceSize offsets[] = { 0 };
                    // Command to bind vertex buffer before drawing with them
                    vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffer, offsets);

                    // Dynamic Offset Amount
                    //uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

                    // Bind mesh index buffer, with zero offset and using the uint32 type
                    vkCmdBindIndexBuffer(commandBuffers[currentImage], thisModel.GetMesh(k)->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage], samplerDescriptorSets[thisModel.GetMesh(k)->getTexId()] };

                    // Bind Descriptor Sets
                    vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

                    // Execute pipline
                    vkCmdDrawIndexed(commandBuffers[currentImage], thisModel.GetMesh(k)->GetIndexCount(), 1, 0, 0, 0);
                }
            }

            // Start second subpass
            vkCmdNextSubpass(commandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipline);
            vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPiplineLayout, 0, 1, &inputDescriptorSets[currentImage], 0, nullptr);
            vkCmdDraw(commandBuffers[currentImage], 3, 1, 0, 0);

        // End Render Pass
        vkCmdEndRenderPass(commandBuffers[currentImage]);

    // Stop recording to command buffer
    result = vkEndCommandBuffer(commandBuffers[currentImage]);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to stop recording a command buffer!");
}

void VulkanRenderer::GetPhysicalDevice()
{
    // Enumerate Physical devices the vkInstance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // If no devices available, then none support Vulkan!
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find any GPUs with Vulkan support!");
    }

    // Get List of Physical Devices
    std::vector<VkPhysicalDevice> devieList(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devieList.data());

    for (const auto &device: devieList)
    {
        if (checkDeviceSuitable(device))
        {
            mainDevice.physicalDevice = device;
            break;
        }
    }

    // Get Property of our new device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

    //minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;

}

void VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
    // calculate alignment of model data
    /*modelUniformAlignment = (sizeof(UboModel) + minUniformBufferOffset - 1) & ~(minUniformBufferOffset -1);

    // Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
#if defined(__linux__)
    modelTransferSpace = (UboModel*)aligned_alloc(modelUniformAlignment, modelUniformAlignment * MAX_OBJECTS);
#else
    modelTransferSpace = (UboModel*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
#endif
    */
}

bool VulkanRenderer::CheckInstanceExtentionSupport(std::vector<const char *> *checkExtentions)
{
    // Need to get number of extentions to create array of correct size to hold extentions
    uint32_t extentionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, nullptr);

    // Create list of VkExtensionProperties using count
    std::vector<VkExtensionProperties> extentions(extentionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, extentions.data());

    // Check if given extentions are in list of avalable extentions
    for (const auto &checkExtentions: *checkExtentions)
    {
        bool hasExtention = false;
        for (const auto &extention: extentions)
        {
            if (strcmp(checkExtentions, extention.extensionName))
            {
                hasExtention = true;
                break;
            }
        }

        if (!hasExtention)
        {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::CheckDeviceExtentionSupport(VkPhysicalDevice device)
{
    // Get Device Extention Count
    uint32_t extentionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extentionCount, nullptr);

    // If no extentions found, return failure
    if (extentionCount == 0)
    {
        return false;
    }

    // Populate list of extentions
    std::vector<VkExtensionProperties> extentions(extentionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extentionCount, extentions.data());

    // Check for extention
    for (const auto &deviceExtention: deviceExtentions)
    {
        bool hasExtention = false;
        for (const auto &extenion: extentions)
        {
            if (strcmp(deviceExtention, extenion.extensionName) == 0)
            {
                hasExtention = true;
                break;
            }
        }

        if (!hasExtention)
        {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
    /*
    // Information about the device itself (ID, name, type, etc)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    */

    // Information about what the Device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = GetQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtentionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapChainDetails swapChainDetails = GetSwapChainDetails(device);
        swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
    }

    return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties: availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

QueueFamilyIndices VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    // Get all Queue Family Property info for the given device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // Go through each queue family and check if it has at least one of the required types of queue
    int i = 0;
    for (const auto &queueFamilie: queueFamilies)
    {
        // First Check if queue family has at least one queue in that family (could have no queues)
        // Queue can be multiple types defined throught bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if it has that type
        if (queueFamilie.queueCount > 0 && queueFamilie.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            // If queue family is valid, then get index
            indices.graphicsFamily = i;
        }

        // Check if queue family supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
        // Check if queue is presenation type (can be both graphics and presentaion)
        if (queueFamilie.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = i;
        }

        // Check if queue family indices are in a valid state, stop searching if so
        if (indices.isValid())
        {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device)
{
    SwapChainDetails swapChainDetails;

    // -- CAPABILITIES --
    // Get the surface capabilities for the given device and surface
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

    // -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // If formats returned, get list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    // If presentation modes returned, get list of presentation modes
    if (presentModeCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                                  swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

// Best Format is subjective but ours will be:
// format		:	VK_FORMAT_R8G8B8A8_UNORM (8 bits for Red, Green, Blue, and Alpha channels with no color space correction (UNORM)) -- (VK_FORMAT_B8G8R8A8_UNORM for backup)
// colorSpace	:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (standard color space for displaying images to screen)
VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // If restricted, search for optimal format
    for (const auto &format: formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace
            == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return formats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
{
    // look for Mailbox presentation mode
    for (const auto &presentationMode: presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }

    // If cant find Mailbox use FIFO which is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities)
{
    // if current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    } else
    {
        // If value can vary, need to set manually

        // Get window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure within boundaries by clamping value
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
                                   std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
                                    std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

        return newExtent;
    }
}

VkFormat VulkanRenderer::ChooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featuresFlags)
{
    // Loop though option and find compatible one
    for (VkFormat format : formats)
    {
        // Get property for given format
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(mainDevice.physicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featuresFlags) == featuresFlags)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featuresFlags) == featuresFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a matching format");
}

VkImage VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usaFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory*imageMemory)
{
    // CREATE IMAGE
    // Image Creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // Type of image (1D, 2D, or 3D)
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    // Width of image extent
    imageCreateInfo.extent.width = width;
    // Height of image extend
    imageCreateInfo.extent.height = height;
    // Depth of image (just 1, no 3D)
    imageCreateInfo.extent.depth = 1;
    // Number of mipmap levels
    imageCreateInfo.mipLevels = 1;
    // Number of levels in image array
    imageCreateInfo.arrayLayers = 1;
    // Format of image
    imageCreateInfo.format = format;
    // How image data should be "tiled" (arranged for optimal reading)
    imageCreateInfo.tiling = tiling;
    // Layout of image data on creation
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Bit flags defining what image will be used for
    imageCreateInfo.usage = usaFlags;
    // Number of samples for multi-sampling
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // Whether image can be shard between queues
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Creating Image
    VkImage image;
    VkResult result = vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create image!");

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined property
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(mainDevice.physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image memory!");

    // Connect Memory to image
    vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0);

    return image;
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // Image to create view for
    viewCreateInfo.image = image;
    // Type of image (e.g. 1D, 2D, 3D, Cube, etc)
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // Format of image data
    viewCreateInfo.format = format;
    // allows remaping of rgba components to other rgba values
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only part of the image

    // Whitch aspect of image to view (e.g. COLOR_BIT for viewing color)
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    // Start mipmap level to view from
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.levelCount = 1;
    // Start array layer to view from
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    // Number of array levels to view
    viewCreateInfo.subresourceRange.layerCount = 1;

    // Create Image View and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image views!");
    }

    return imageView;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char> &code)
{
    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Size of code
    shaderModuleCreateInfo.codeSize = code.size();
    // Pointer to code (of uint32_t pointer type)
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

int VulkanRenderer::CreateTextureImage(std::string filename)
{
    // Load image file
    int width, height;
    VkDeviceSize imageSize;
    stbi_uc* imageData = LoadTextureFile(filename, &width, &height, &imageSize);

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer imageStagingBuffer;
    VkDeviceMemory imageStagingBufferMemory;
    CreateBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageStagingBuffer, &imageStagingBufferMemory);

    // Copy image data to staging buffer
    void* data;
    vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);

    // Free original image data
    stbi_image_free(imageData);

    // Create Image to hold final texture
    VkImage texImage;
    VkDeviceMemory texImageMemory;
    texImage = CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

    // COPY DATA TO IMAGE
    // Transition image to be DST for copy operation
    TransitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy image data
    CopyImageBuffer(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width, height);

    // Transition image to be shader readable for shader usage
    TransitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Add texture data to vector for reference
    textureImages.push_back(texImage);
    textureImageMemory.push_back(texImageMemory);

    // Destroy Staging buffers
    vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

    //  Return index of new texture image
    return textureImages.size() - 1;
}

int VulkanRenderer::CreateTexture(std::string filename)
{
    // Create Texture image and gets is location in array
    int textureImageLoc = CreateTextureImage(filename);

    // Create Image view and add to list
    VkImageView imageView = CreateImageView(textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM,  VK_IMAGE_ASPECT_COLOR_BIT);
    textureImageViews.push_back(imageView);

    // Create Texture descriptor
    int descriptorLoc = CreateTextureDescriptor(imageView);

    // Return location of set with texture
    return descriptorLoc;
}

int VulkanRenderer::CreateTextureDescriptor(VkImageView textureImage)
{
    VkDescriptorSet descriptorSet;

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = samplerDescriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &samplerSetLayout;

    // Allocate Descriptor Sets
    VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, &descriptorSet);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate Texture descriptor set!");

    // Texture Image Info
    VkDescriptorImageInfo imageInfo = {};
    // Image layout when in use
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Image to bind to set
    imageInfo.imageView = textureImage;
    // Sampler to use for set
    imageInfo.sampler = textureSampler;

    // Descriptor Write Info
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    // Update new descriptor set
    vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

    // Add descriptor set to list
    samplerDescriptorSets.push_back(descriptorSet);

    // Return descriptor set location
    return samplerDescriptorSets.size() - 1;

}

int VulkanRenderer::CreateMeshModel(std::string modelFile)
{
    // Import model "Scene"
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene)
        throw std::runtime_error("Failed to load model! (" + modelFile + ")");

    // Get vector of all materials with 1 : 1 ID placement
    std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

    // Convertion from the materials list IDs to our Descriptor Array IDs
    std::vector<int> matToTex(textureNames.size());

    // Loop over textureNames and create textures for them
    for (size_t i = 0; i < textureNames.size(); i++)
    {
        // If material has no texture, set '0' to indicate no texture, texture 0 will be reserved for a default texture
        if (textureNames[i].empty())
            matToTex[i] = 0;
        else
            // Otherwise, create texture and set value to index of new texture
            matToTex[i] = CreateTexture(textureNames[i]);
    }

    // Load in all our meshes
    std::vector<Mesh> modelMeshes = MeshModel::LoadNode(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, scene->mRootNode, scene, matToTex);

    // Create Mesh model and add to list
    MeshModel meshModel = MeshModel(modelMeshes);
    modelList.push_back(meshModel);

    return modelList.size() - 1;
}

stbi_uc * VulkanRenderer::LoadTextureFile(std::string fileName, int *width, int *height, VkDeviceSize *imageSize)
{
    // Number of channels image uses
    int channels;

    // Load pixel data for image
    std::string fileLoc = "Textures/" + fileName;
    stbi_uc * image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

    if (!image)
        throw std::runtime_error("Failed to load a texture file! (" + fileName + ")");

    // Calculate image size using given and known data
    *imageSize = *width * *height * 4;

    return image;
}
