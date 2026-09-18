#pragma once

#include <string>

#include <renderer/Renderer.h>

struct GLFWwindow;

class Window
{
public:
    enum WindowMode
    {
        Windowed,
        Fullscreen,
        Borderless
    };

    struct Desc
    {
        std::string name = "Default";
        int width = 1280;
        int height = 720;
        bool resizable = false;

        WindowMode mode = WindowMode::Windowed;
        Renderer::Backend backend = Renderer::Backend::Vulkan;
    };

    Window(const Desc& _desc);
    ~Window();

    // Makes it so you can not duplicate the window
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents();

    GLFWwindow* Handle() const
    {
        return handle;
    }

    const Desc* GetDesc() const
    {
        return &desc;
    }

private:
    GLFWwindow* handle = nullptr;
    Desc desc;
};
