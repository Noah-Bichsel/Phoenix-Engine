#pragma once
#include <application/ILayer.h>

class CoreLayer : public ILayer
{
public:
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
};

