#pragma once

#include <string>
#include <memory>
#include "Scene.h"

namespace Phoenix
{
    class SceneManager
    {
    public:
        static Scene& CreateScene(std::string name = "Untitled")
        {
            s_activeScene = std::make_unique<Scene>();
            return *s_activeScene;
        }

        static Scene* GetActiveScene()
        {
            return s_activeScene.get();
        }

        static void SetActiveScene(std::unique_ptr<Scene> scene)
        {
            s_activeScene = std::move(scene);
        }

    private:
        inline static std::unique_ptr<Scene> s_activeScene = nullptr;
    };
}