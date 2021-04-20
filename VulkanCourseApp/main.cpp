#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#define VULKAN_RENDERER_NEW

#include "WindowLVE.h"

#if defined(VULKAN_RENDERER_NEW)
#include "VulkanRendererNew.h"
#else
#include "VulkanRenderer.h"
#endif

std::shared_ptr<WindowLVE> window;

#if defined(VULKAN_RENDERER_NEW)
std::unique_ptr<VulkanRendererNew> vulkanRenderer;
#else
std::unique_ptr<VulkanRenderer> vulkanRenderer;
#endif

void initWindow(std::string wName = "Vulkan Renderer", const int width = 1280, const int height = 720)
{
	window = std::make_shared<WindowLVE>(width, height, wName);
}

int main()
{
	// Create Window
	initWindow("Vulkan Renderer", 1366, 768);

	// Create VulkanRenderer instance
#if defined(VULKAN_RENDERER_NEW)
	vulkanRenderer = std::make_unique<VulkanRendererNew>(window);
#else
	vulkanRenderer = std::make_unique<VulkanRenderer>(window);
#endif

	if (vulkanRenderer->init() == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	int meshId1 = vulkanRenderer->createMeshModel("Models/cyborg.obj");
	int meshId2 = vulkanRenderer->createMeshModel("Models/cyborg.obj");

	// Loop until closed
	while (!window->shouldClose())
	{
		glfwPollEvents();

		float now = (float)glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 20.0f * deltaTime;
		if (angle > 360.0f) { angle -= 360.0f; }

		glm::mat4 modelMat1(1.0f);
		modelMat1 = glm::translate(modelMat1, glm::vec3(-9.5f, 0.0f, 0.0f));
		modelMat1 = glm::rotate(modelMat1, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMat1 = glm::scale(modelMat1, glm::vec3(5.0f));

		glm::mat4 modelMat2(1.0f);
		modelMat2 = glm::translate(modelMat2, glm::vec3( 9.5f, 0.0f, 0.0f));
		modelMat2 = glm::rotate(modelMat2, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMat2 = glm::scale(modelMat2, glm::vec3(5.0f));

		vulkanRenderer->updateModel(meshId1, modelMat1);
		vulkanRenderer->updateModel(meshId2, modelMat2);

		vulkanRenderer->draw();
	}

	return EXIT_SUCCESS;
}
