#include "VulkanRenderer.h"
#include <cstring>
#include <limits>

int VulkanRenderer::init(GLFWwindow *newWindow)
{
    window = newWindow;

    try
    {
        // needs to be in this order
        CreateInstance();
        setupDebugMessenger();
        CreateSurface(); 
        GetPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        RecordCommands();
        CreateSyncronization();
    }
    catch (const std::runtime_error &e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void VulkanRenderer::Draw()
{
    // -- GET NEXT IMAGE --
    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(mainDevice.logicalDevice, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable, VK_NULL_HANDLE, &imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue Submission information
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // Number of semaphores to wait on
    submitInfo.waitSemaphoreCount = 1;
    // List of semaphores to wait on
    submitInfo.pWaitSemaphores = &imageAvailable;
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
    submitInfo.pSignalSemaphores = &renderFinished;

    // Submit command buffer to the queue
    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
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
    presentInfo.pWaitSemaphores = &renderFinished;
    // Number of swapchians to present to
    presentInfo.swapchainCount = 1;
    // swapchians to present images to
    presentInfo.pSwapchains = &swapChain;
    // Index of images in swapchains to present
    presentInfo.pImageIndices = &imageIndex;

    // Present Image
    result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Image!");
    }
}

void VulkanRenderer::cleanup()
{
    vkDestroySemaphore(mainDevice.logicalDevice, renderFinished, nullptr);
    vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable, nullptr);
    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
    for (auto framebuffer: swapChainFramebuffers)
        vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
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

    // Set up validation layers that Instance will use
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    // Set up debug messenger create info and add to instance create info
    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;

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

    // TODO: set up Validation layers that Instance will use (for debugging)
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

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
    // Number of enabled logical device extentions
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtentions.size());
    // List of enabled logical device extentions
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtentions.data();

    // Physical Device Features that the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures{};
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

void VulkanRenderer::setupDebugMessenger()
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
    // color attachment of render pass
    VkAttachmentDescription colorAttachment = {};
    // Format to use for attachment
    colorAttachment.format = swapChainImageFormat;
    // Number of samples to write for multisampling
    colorAttachment.samples = VK_SAMPLE_COUNT_16_BIT;
    // Describes what to do with attachment before rendering
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // Describes what to do with attachment after rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Describes what to do with stencil for rendering
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // Describes what to do with stencil after rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Framebuffer data will be stored as an image, but images can be given differnt data layouts to give optimal use for certain opertaions
    // Image data layout before render pass starts
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Image data layout after render pass (to change to)
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    // Pipeline type subpass is to be bound to
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    // Need to determaine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Converstion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

    // Converstion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
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

    // -- VERTEX INPUT (TODO: Put in vertex descriptions when resources created) --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    // List of vertex Binding Descripts (data spacing/stride informaion)
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    // List of vertex Attribute Desritions (data format and where to bind to/from)
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // Primative type to assemble vertices as
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
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Whether to add depth bias to fragmesnts (good for stopping "shadow acne" when using shadow maps)
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampl shadeing or not (for antialiasing, etc)
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
    // Alternative to calcualtion is to use logical operations
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorState;

    // -- PIPELINE LAYOUT (TODO: apply Future Descriptor Set Layouts) --
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr,
                                             &pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // -- DEPTH STENCIL TESTING --
    // TODO: Set up depth stencil testing

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
    pipelineCreateInfo.pDepthStencilState = nullptr;
    // Pipeline layout pipleline should use
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
}

void VulkanRenderer::CreateFramebuffers()
{
    // Resize frame buffer count to equal swap chian image count
    swapChainFramebuffers.resize(swapChainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        std::array<VkImageView, 1> attachments
        {
            swapChainImages[i].imageView
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
    // Semaphore Creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create semaphore!");
    }
}

void VulkanRenderer::RecordCommands()
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Buffer can be resubmitted when has already been submitted and is awaiting execution
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Information abot how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // Render pass to begin
    renderPassBeginInfo.renderPass = renderPass;
    // Start of point of render pass in pixels
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    // Size of region to run render pass on (starting at offset)
    renderPassBeginInfo.renderArea.extent = swapChainExtent;
    VkClearValue clearValues[] =
    {
        {0.6f, 0.65f, 0.04f, 1.0f}
    };
    // List of clear values (TODO: Depth attachment Clear Value)
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.clearValueCount = 1;

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];

        // Start recording commands to command buffer
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to begin command buffer!");

            // Begin Render Pass
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Bind Pipeline to be used in render pass
                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                // Execute pipline
                vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

            // End Render Pass
            vkCmdEndRenderPass(commandBuffers[i]);

        // Stop recording to command buffer
        result = vkEndCommandBuffer(commandBuffers[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to stop recording a command buffer!");
    }
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

    // Information about what the Device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */

    QueueFamilyIndices indices = GetQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtentionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapChainDetails swapChainDetails = GetSwapChainDetails(device);
        swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
    }

    return indices.isValid() && extensionsSupported && swapChainValid;
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
