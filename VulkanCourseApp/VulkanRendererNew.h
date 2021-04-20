#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include "Utilities.h"
#include "Mesh.h"
#include "MeshModel.h"
#include "WindowLVE.h"
#include "SwapChainVCA.h"
#include "PipelineLVE.h"

#include <vector>


class VulkanRendererNew
{
public:
	VulkanRendererNew() = delete;
	VulkanRendererNew(std::shared_ptr<WindowLVE> window);
	~VulkanRendererNew();

	int init();
	int createMeshModel(std::string modelFile);
	void updateModel(int modelId, glm::mat4 newModel);
	void draw();
	void cleanup();

	void recreateSwapChainLVE();
	void recordCommandBufferLVE(int imageIndex);
	void renderGameObjectsLVE(VkCommandBuffer commandBuffer);
	void drawFrameLVE();

private:
	std::shared_ptr<WindowLVE> m_Window; // lveWindow
	std::shared_ptr<DeviceLVE> m_Device; // lveDevice
	std::unique_ptr<SwapChainVCA> m_SwapChain; // lveSwapChain
	std::unique_ptr<PipelineLVE> m_Pipeline; // lvePipeline

	int currentFrame = 0;

	// Scene Objects
	std::vector<MeshModel> modelList;

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

	// struct
	// {
	// 	VkPhysicalDevice physicalDevice;
	// 	VkDevice logicalDevice;
	// } mainDevice;

	// VkQueue graphicsQueue;
	// VkQueue presentationQueue;
	// VkSurfaceKHR surface;
	// VkSwapchainKHR swapchain;

	// std::vector<SwapChainVCA::SwapchainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkImage> colorBufferImages;
	std::vector<VkDeviceMemory> colorBufferImageMemory;
	std::vector<VkImageView> colorBufferImageViews;

	std::vector<VkImage> depthBufferImages;
	std::vector<VkDeviceMemory> depthBufferImageMemory;
	std::vector<VkImageView> depthBufferImageViews;

	VkSampler textureSampler;

	// -- Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSetLayout inputSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	VkDescriptorPool inputDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;
	std::vector<VkDescriptorSet> inputDescriptorSets;

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	// std::vector<VkBuffer> modelDynUniformBuffer;
	// std::vector<VkDeviceMemory> modelDynUniformBufferMemory;

	// VkDeviceSize minUniformBufferOffset;
	// size_t modelUniformAlignment;
	// Model* modelTransferSpace;

	// -- Assets
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// -- Pipelines
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	VkPipeline secondPipeline;
	VkPipelineLayout secondPipelineLayout;

	VkRenderPass renderPass;

	// -- Pools --
	VkCommandPool graphicsCommandPool;

	// -- Utility
	// VkFormat swapChainImageFormat;
	// VkExtent2D swapChainExtent;

	// -- Synchronization
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	// Vulkan Functions
	// -- Create Functions
	void createInstance();
	void createDebugCallback();
	// void createLogicalDevice();
	// void createSurface();
	void createDevice();
	void createSwapChain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createColorBufferImage();
	void createDepthBufferImage();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();
	void createTextureSampler();

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createInputDescriptorSets();

	void freeCommandBuffers();

	void updateUniformBuffers(uint32_t imageIndex);

	// -- Record Functions --
	void recordCommands(uint32_t currentImage);

	// -- Get Functions
	// void getPhysicalDevice();

	// -- Allocate Functions
	void allocateDynamicBufferTransferSpace();

	// -- Support Functions
	// -- -- Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(); // VkPhysicalDevice device
	bool checkValidationLayerSupport();
	bool checkDeviceSuitable(); // VkPhysicalDevice device

	// -- Getter Functions
	// QueueFamilyIndices getQueueFamilies(); // VkPhysicalDevice device
	// SwapChainLVE::SwapChainDetails getSwapChainDetails(); // VkPhysicalDevice device

	// -- Choose Functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// -- Create Functions
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char> &code);

	int createTextureImage(std::string fileName);
	int createTexture(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	// -- Loader Functions
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);

};
