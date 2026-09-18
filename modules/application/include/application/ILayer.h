#pragma once

class ILayer
{
public:
    ILayer() = default;
    virtual ~ILayer() = default;

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;
    virtual void OnUpdate() = 0;
};