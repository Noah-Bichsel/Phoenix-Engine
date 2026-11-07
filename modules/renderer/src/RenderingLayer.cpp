#include "../include/renderer/RenderingLayer.h"

#include <application/Application.h>
#include <ecs/Scene.h>
#include <ecs/Component.h>
#include <core/math/GlmInterop.h>

#include <glm/ext/matrix_transform.hpp>

#include "ecs/SceneManager.h"

using namespace Phoenix;

void RenderingLayer::OnAttach()
{
    renderer = Renderer::Create(Application::Get().GetWindow()->Handle(), Application::Get().GetWindow()->GetDesc()->backend);

    if (Scene* scene = SceneManager::GetActiveScene())
    {
        auto view = scene->GetRegistry().view<TransformComponent, MeshComponent>();
        for (auto [name, transform, mesh] : view.each())
        {
            mesh.m_id = renderer->CreateMeshModel(mesh.m_path);
        }
    }
}

void RenderingLayer::OnDetach()
{
    renderer->cleanup();
}

void RenderingLayer::OnUpdate()
{
    if (Scene* scene = SceneManager::GetActiveScene())
    {
        auto view = scene->GetRegistry().view<TransformComponent, MeshComponent>();
        for (auto [name, transform, mesh] : view.each())
        {
            glm::mat4 model(1.0f);
            model = glm::scale(glm::mat4(1.0f), ToGlm(transform.m_scale));
            model = glm::translate(model, ToGlm(transform.m_position));
            model = glm::rotate(model, glm::radians(transform.m_rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(transform.m_rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(transform.m_rotation.z), glm::vec3(0, 0, 1));
            renderer->updateModel(mesh.m_id, model);
        }
    }

    renderer->Draw();
}
