#pragma once

#include "Object.h"

struct Behaviour
{
    Object object;

    bool enabled{true};

    Transform &Transform() const { return object.Transform(); }
};

template <typename T>
concept HasStart = requires(T &script) { script.Start(); };

template <typename T>
concept HasUpdate = requires(T &script) { script.Update(); };

template <typename T>
concept HasOnUpdate = requires(T &script) { script.OnUpdate(); };