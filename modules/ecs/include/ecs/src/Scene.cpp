#include "../Scene.h"
#include "../Object.h"

using namespace Phoenix;

Object Scene::CreateObject(const std::string& name)
{
    Object object(m_registry.create(), this);
    // Required/Default components
    object.AddComponent<NameComponent>(name);
    object.AddComponent<TransformComponent>();
    return object;
}

void Scene::Destroy(Object object)
{
    if (object)
    {
        m_registry.destroy(object.GetEntity());
    }
}
