#pragma once

#include <entt/entity/registry.hpp>

#include "Component.h"

#include <memory>

class Scene;

class Object
{
public:
    Object() = default;
    Object(Scene* scene, entt::entity entity) : scene(scene), entity(entity) {}

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template <typename T>
    T* GetComponent() const;

    Transform& Transform() const;
    std::string& Name() const;

    Scene &GetScene() const { return *scene; }
    entt::entity GetEntity() const { return *entity; }

private:
    std::shared_ptr<Scene> scene;
    entt::entity entity {entt::null};
};