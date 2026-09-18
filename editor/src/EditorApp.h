#pragma once

#include <application/Application.h>

#include <memory>
#include <array>

#include <engine/CoreLayer.h>
#include <engine/RenderingLayer.h>

class EditorApp : public Application
{
public:
    EditorApp(const Window::Desc& desc);

private:

    // The Layers that are run (in order)
    std::array<std::unique_ptr<ILayer>, 2> Layers =
    {
        std::make_unique<CoreLayer>(),
        std::make_unique<RenderingLayer>(),
    };
};