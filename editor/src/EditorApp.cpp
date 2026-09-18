#include "EditorApp.h"
EditorApp::EditorApp(const Window::Desc& desc) : Application(desc)
{
    for (auto& layer : Layers)
       layerStack->PushLayer(std::move(layer));
}
