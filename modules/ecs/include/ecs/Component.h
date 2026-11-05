#pragma once

#include <string>
#include <vector>
#include <entt/entity/entity.hpp>

struct Name
{
    std::string name;
};

struct Vector2
{
    float x{}, y{};
};

struct Vector3
{
    float x{}, y{}, z{};
};

struct Transform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

    entt::entity parent {entt::null};
    std::vector<entt::entity> children;
};

struct Mesh
{

};