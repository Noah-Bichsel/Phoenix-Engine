#pragma once

#include <memory>
#include <vector>
#include <stdexcept>
#include <span>

#include "ILayer.h"

class LayerStack
{
public:
    LayerStack()
    {
        if (instance)
            throw std::logic_error("LayerStack already exists");
        instance = this;
    }

    ~LayerStack() { instance = nullptr; }

    LayerStack(const LayerStack&) = delete;
    LayerStack& operator=(const LayerStack&) = delete;

    static LayerStack& Get()
    {
        return *instance;
    }

    void PushLayer(std::unique_ptr<ILayer> layer);
    void PopLayer(ILayer* layer);

    void Update();

    std::span<const std::unique_ptr<ILayer>> GetLayers() const
    {
        return layerStack;
    }

private:
    static inline LayerStack* instance = nullptr;

    std::vector<std::unique_ptr<ILayer>> layerStack;
};
