#include "EditorApp.h"

#include <ecs/SceneManager.h>
#include <ecs/Scene.h>
#include <ecs/Object.h>

using namespace Phoenix;

EditorApp::EditorApp(const Window::Desc& desc) : Application(desc)
{
    Scene& scene = SceneManager::CreateScene("Sample Scene");
    Object tree = scene.CreateObject("Tree");
    tree.AddComponent<MeshComponent>("Models/10446_Palm_Tree_v1_max2010_iteration-2.obj");
    auto* transform = tree.GetComponent<TransformComponent>();
    transform->m_scale = Vector3(0.1f, 0.1f, 0.1f);
    transform->m_rotation = Vector3(-90.0f, 0.0f, 0.0f);
    transform->m_position = Vector3(0.0f, -350.0f, 200.0f);


    for (auto& layer : Layers)
       layerStack->PushLayer(std::move(layer));
}
