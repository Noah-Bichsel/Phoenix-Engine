#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>

struct GLFWwindow;

class Renderer
{
public:
    enum Backend
    {
        Vulkan,
    };

    virtual ~Renderer() = default;

    static std::unique_ptr<Renderer> Create(GLFWwindow* window, Backend backend);

    virtual int CreateMeshModel(std::string modelFile) = 0;
    virtual void updateModel(int modelId, glm::mat4 newModel) = 0;

    virtual void Draw() = 0;
    virtual  void cleanup() = 0;
};