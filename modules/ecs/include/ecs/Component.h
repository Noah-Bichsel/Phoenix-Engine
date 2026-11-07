#pragma once

#include <string>
#include <vector>
#include <entt/entity/entity.hpp>
#include <core/math/Vector.h>

namespace Phoenix
{
    struct NameComponent
    {
        std::string m_name;
    };

    struct TransformComponent
    {
        Vector3 m_position = Vector3::Zero();
        Vector3 m_rotation = Vector3::Zero();
        Vector3 m_scale = Vector3(1.0f, 1.0f, 1.0f);

        entt::entity parent {entt::null};
        std::vector<entt::entity> children;
    };

    struct MeshComponent
    {
        std::string m_path{};
        int m_id = -1;

        MeshComponent() = default;
        MeshComponent(const std::string& path) : m_path(path) {}
    };
}