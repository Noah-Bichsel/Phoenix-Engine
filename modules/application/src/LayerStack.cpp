#include "../include/application/LayerStack.h"

void LayerStack::PushLayer(std::unique_ptr<ILayer> layer)
{
    if (!layer)
        throw std::invalid_argument("PushLayer: null layer");

    layerStack.push_back(std::move(layer));
    layerStack.back()->OnAttach();
}

void LayerStack::PopLayer(ILayer* layer)
{
    layer->OnDetach();
    std::erase_if(layerStack, [layer](const auto& p) { return p.get() == layer; });
}

void LayerStack::Update()
{
    for (const auto& layer : layerStack)
    {
        layer->OnUpdate();
    }
}
