#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Utilities.h"
#include "Mesh.h"

#include <vector>


class VulkanRenderer
{
public:
	VulkanRenderer();
	int init(GLFWwindow* newWindow);
	void updateModel(glm::mat4 newModel);
	void draw();
	void cleanup();
	~VulkanRenderer();

private:
	GLFWwindow* window;

	int currentFrame = 0;

	// Scene Objects
	std::vector<Mesh> meshList;

	// Scene Settings
	struct UboViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	// Vulkan Components
	// -- Main
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	// -- Descriptors
	VkDescriptorSetLayout descriptorSetLayout;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	std::vector<VkBuffer> modelDynUniformBuffer;
	std::vector<VkDeviceMemory> modelDynUniformBufferMemory;

	VkDeviceSize minUniformBufferOffset;
	size_t modelUniformAlignment;
	UboModel* modelTransferSpace;

	// -- Pipeline
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	// -- Pools --
	VkCommandPool graphicsCommandPool;

	// -- Utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// -- Synchronization
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	// Vulkan Functions
	// -- Create Functions
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffers(uint32_t imageIndex);

	// -- Record Functions --
	void recordCommands();

	// -- Get Functions
	void getPhysicalDevice();

	// -- Allocate Functions
	void allocateDynamicBufferTransferSpace();

	// -- Support Functions
	// -- -- Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// -- Getter Functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	// -- Choose Functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	// -- Create Functions
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char> &code);
	void createSynchronization();

};
