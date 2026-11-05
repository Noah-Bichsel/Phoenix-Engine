#pragma once

#include <string>

#include <entt/entt.hpp>

#include "Object.h"

class Scene
{
public:
    Object Create(std::string name);

    void Destroy(Object object);

    entt::registry& GetRegistry()
    {
        return registry;
    }

private:
    entt::registry registry;
};