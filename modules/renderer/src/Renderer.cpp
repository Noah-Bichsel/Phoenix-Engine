#include <renderer/Renderer.h>
#include "vulkan/vk_renderer.h"

std::unique_ptr<Renderer> Renderer::Create(GLFWwindow* window, Backend backend)
{
    switch (backend)
    {
        case Backend::Vulkan:
        {
            auto vk = std::make_unique<vk_renderer>();
            if (vk->init(window) == EXIT_FAILURE)
                throw std::runtime_error("Failed to initialize Vulkan");
            return vk;
        }
        default:
            return nullptr;
    }
};
