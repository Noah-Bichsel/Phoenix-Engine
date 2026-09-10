#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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

	int helicopter = vulkanRenderer.CreateMeshModel("Models/Seahawk.obj");

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

		glm::mat4 testMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
		testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		testMat = glm::translate(testMat, glm::vec3(0.0f, -15.0f, 0.0f));
		vulkanRenderer.updateModel(helicopter, testMat);

		vulkanRenderer.Draw();
	}

	vulkanRenderer.cleanup();

	// Destroys GLFW window
	glfwDestroyWindow(window);
	// Stops GLFW
	glfwTerminate();

	return 0;
}