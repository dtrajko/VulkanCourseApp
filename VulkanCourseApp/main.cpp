#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "WindowLVE.h"
#include "VulkanRenderer.h"
#include "CameraController.h"

std::shared_ptr<WindowLVE> window;
std::unique_ptr<VulkanRenderer> vulkanRenderer;

std::shared_ptr<Camera> camera;
std::unique_ptr<CameraController> cameraController;

void initWindow(std::string wName = "Vulkan Renderer", const int width = 1280, const int height = 720)
{
	window = std::make_shared<WindowLVE>(width, height, wName);
	camera = std::make_shared<Camera>(glm::vec3(0.0f, 10.0f, 40.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
	cameraController = std::make_unique<CameraController>(camera, 1.778f, 4.0f, 0.1f);
}

int main()
{
	// Create Window
	initWindow("Vulkan Renderer", 1366, 768);

	vulkanRenderer = std::make_unique<VulkanRenderer>(window);

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

		camera->OnUpdate(deltaTime);
		cameraController->Update(deltaTime, window->getHandle());
		vulkanRenderer->update(deltaTime, camera);

		vulkanRenderer->draw();
	}

	return EXIT_SUCCESS;
}
