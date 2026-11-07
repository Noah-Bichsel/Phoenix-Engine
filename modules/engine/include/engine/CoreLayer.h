#pragma once

#include <application/ILayer.h>

namespace Phoenix
{
    class CoreLayer : public ILayer
    {
    public:
        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate() override;
    };
}

