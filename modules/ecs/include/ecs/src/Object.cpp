#include "../Object.h"
#include "../Behaviour.h"
#include "../Scene.h"

template<typename T>
T *Object::GetComponent() const
{
    return scene->registry.try_get<T>(entity);
}

template<typename T, typename... Args>
T &Object::AddComponent(Args &&... args)
{
    auto &component = scene->registry.try_get<T>(entity, std::forward<Args>(args)...);

    if constexpr(std::is_base_of_v<Behaviour, T>)
    {
        component.object = *this;
    }

    return component;
}
