#pragma once

#include <stdexcept>

#include <application/LayerStack.h>

#include "Window.h"

class Application
{
protected:
    Application(const Window::Desc& desc) : window(std::make_unique<Window>(desc))
    {
        if (instance)
            throw std::logic_error("Application already exists");
        instance = this;
    }

public:
    virtual ~Application()
    {
        instance = nullptr;
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static Application& Get()
    {
        return *instance;
    }

    static bool exists()
    {
        return instance != nullptr;
    }

    Window* GetWindow() const
    {
        return window.get();
    }

    inline void Run()
    {
        // main loop
        while (!window->ShouldClose())
        {
            // GLFW events (inputs)
            window->PollEvents();
            // call update on each layer in the layer stack
            layerStack->Update();
        }

        Shutdown();
    }

protected:
    virtual void Shutdown()
    {
        // Call the Detach function for all layers
        for (auto& layer : layerStack->GetLayers())
        {
            layer->OnDetach();
        }
    }

    std::unique_ptr<LayerStack> layerStack = std::make_unique<LayerStack>();
    std::unique_ptr<Window> window;

private:
    static inline Application* instance = nullptr;
};
Application* CreateApplication();
