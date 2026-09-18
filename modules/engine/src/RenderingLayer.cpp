#include "../include/engine/RenderingLayer.h"

#include <application/Application.h>

void RenderingLayer::OnAttach()
{
    renderer = Renderer::Create(Application::Get().GetWindow()->Handle(), Application::Get().GetWindow()->GetDesc()->backend);
}

void RenderingLayer::OnDetach()
{
    renderer->cleanup();
}

void RenderingLayer::OnUpdate()
{
    renderer->Draw();
}
