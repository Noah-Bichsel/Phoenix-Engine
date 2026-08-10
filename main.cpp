#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vulkanRenderer;

void InitWindow(std::string wName = "Test Window", const int width = 800, const int hight = 600)
{
#if defined(__linux__)
	// Since we are on linux
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
	// Initialize GLFW
	glfwInit();

	// Tell GLFW not to use OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// Tell GLFW to not allow the window to be resized
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Create the window
	window = glfwCreateWindow(width, hight, wName.c_str(), nullptr, nullptr);
}

int main()
{
	//Create window
	InitWindow();

	// Create Vulkan Renderer instance
	if (vulkanRenderer.init(window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	// loop until the user closes the window
	while (!glfwWindowShouldClose(window))
	{
		// Poll for and process events
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;

		if (angle > 360.0f)
			angle -= 360.0f;

		glm::mat4 firstModel(1.0f);
		glm::mat4 secondModel(1.0f);

		firstModel = glm::translate(firstModel, glm::vec3(-2.0f, 0.0f, -5.0f));
		firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		secondModel = glm::translate(secondModel, glm::vec3(2.0f, 0.0f, -5.0f));
		secondModel = glm::rotate(secondModel, glm::radians(-angle*100), glm::vec3(0.0f, 0.0f, -1.0f));

		vulkanRenderer.updateModel(0, firstModel);
		vulkanRenderer.updateModel(1, secondModel);

		vulkanRenderer.Draw();
	}

	vulkanRenderer.cleanup();

	// Destroys GLFW window
	glfwDestroyWindow(window);
	// Stops GLFW
	glfwTerminate();

	return 0;
}