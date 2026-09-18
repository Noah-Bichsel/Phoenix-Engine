#pragma once

#include "application/ILayer.h"
#include "renderer/Renderer.h"

class RenderingLayer : public ILayer
{
public:
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;

private:
    std::unique_ptr<Renderer> renderer;
};
