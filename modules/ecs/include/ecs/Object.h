#pragma once

#include <entt/entity/registry.hpp>

#include "Component.h"
#include "Scene.h"

namespace Phoenix
{
    class Scene;

    class Object
    {
    public:
        Object() = default;
        Object(entt::entity entity, Scene* scene) : m_entity(entity), m_scene(scene) {}

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            return m_scene->GetRegistry().emplace_or_replace<T>(m_entity, std::forward<Args>(args)...);
        }

        template <typename T>
        T* GetComponent()
        {
            return m_scene->GetRegistry().try_get<T>(m_entity);
        }

        template<typename T>
        bool HasComponent() const {
            return m_scene->GetRegistry().all_of<T>(m_entity);
        }

        template<typename T>
        void RemoveComponent() {
            m_scene->GetRegistry().remove<T>(m_entity);
        }

        // Built-in function - since all objects will these components
        std::string& GetName()
        {
            return m_scene->GetRegistry().get<NameComponent>(m_entity).m_name;
        }

        TransformComponent& GetTransform()
        {
            return m_scene->GetRegistry().get<TransformComponent>(m_entity);
        }

        explicit operator bool() const {
            return m_scene != nullptr && m_entity != entt::null && m_scene->GetRegistry().valid(m_entity);
        }

        entt::entity GetEntity() const { return m_entity; }

    private:
        Scene* m_scene{nullptr};
        entt::entity m_entity {entt::null};
    };
}
