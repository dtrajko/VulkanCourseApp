#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.h"

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
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;


	// Vulkan Functions
	// -- Create Functions
	void createInstance();

	// -- Get Functions
	void getPhysicalDevice();

	// -- Support Functions
	// -- -- Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// -- Getter Functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

};
