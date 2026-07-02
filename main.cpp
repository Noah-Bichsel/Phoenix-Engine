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
#if defined(__LINUX__)
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

	// loop until the user closes the window
	while (!glfwWindowShouldClose(window))
	{
		// Poll for and process events
		glfwPollEvents();
		vulkanRenderer.Draw();
	}

	vulkanRenderer.cleanup();

	// Destorys GLFW window
	glfwDestroyWindow(window);
	// Stops GLFW
	glfwTerminate();

	return 0;
}