#include "../include/application/Window.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

Window::Window(const Desc& _desc) : desc(_desc)
{
#if defined(__linux__)
    // Since we are on linux
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

    // Initialize GLFW
    if (!glfwInit())
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // gets main monitor
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    if (!mon)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to get monitor");
    }

    // gets monitors settings (resolution, refresh rate)
    const GLFWvidmode* vm = glfwGetVideoMode(mon);
    if (!vm)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to get video mode");
    }

    switch (desc.backend)
    {
        case Renderer::Backend::Vulkan:
            // Tell GLFW not to use OpenGL
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            break;
    }

    switch (desc.mode)
    {
        case WindowMode::Windowed:
            if (!desc.resizable)
                glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            handle = glfwCreateWindow(desc.width, desc.height, desc.name.c_str(), nullptr, nullptr);
            if (!handle)
            {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }
            break;
        case WindowMode::Fullscreen:

            handle = glfwCreateWindow(vm->width, vm->height, desc.name.c_str(), mon, nullptr);
            if (!handle)
            {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }
            break;
        case WindowMode::Borderless:
            // match the desktop mode so no mode switch happens
            glfwWindowHint(GLFW_RED_BITS,     vm->redBits);
            glfwWindowHint(GLFW_GREEN_BITS,   vm->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS,    vm->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, vm->refreshRate);
            glfwWindowHint(GLFW_DECORATED,    GLFW_FALSE);
            glfwWindowHint(GLFW_VISIBLE,      GLFW_FALSE);

            handle = glfwCreateWindow(vm->width, vm->height, desc.name.c_str(), nullptr, nullptr);
            if (handle)
            {
                int mx, my;
                glfwGetMonitorPos(mon, &mx, &my);
                glfwSetWindowPos(handle, mx, my);
                glfwShowWindow(handle);
            }
            else
            {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }
            break;
    }
}

Window::~Window()
{
    // Destroys GLFW window
    glfwDestroyWindow(handle);
    // Stops GLFW
    glfwTerminate();
}

bool Window::ShouldClose() const
{
    if (!glfwWindowShouldClose(handle))
        return false;

    return true;
}

void Window::PollEvents()
{
    glfwPollEvents();
}
