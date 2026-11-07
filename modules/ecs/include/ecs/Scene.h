#pragma once

#include <string>

#include <entt/entt.hpp>

namespace Phoenix
{
    class Object;

    class Scene
    {
    public:
        Object CreateObject(const std::string& name);

        void Destroy(Object object);

        entt::registry& GetRegistry()
        {
            return m_registry;
        }

    private:
        entt::registry m_registry;
    };
}