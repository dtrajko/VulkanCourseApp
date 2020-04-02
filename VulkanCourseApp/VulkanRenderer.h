#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>


class VulkanRenderer
{
public:
	VulkanRenderer();
	int init(GLFWwindow* newWindow);
	void cleanup();
	~VulkanRenderer();

private:
	GLFWwindow* window;

	// Vulkan Components
	VkInstance instance;


	// Vulkan Functions
	// -- Create Functions
	void createInstance();

	// -- Support Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);

};
